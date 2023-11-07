#include "backends/p4tools/modules/flay/core/parser_stepper.h"

#include <stddef.h>

#include <string>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/gen_eq.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

ParserStepper::ParserStepper(FlayStepper &stepper) : stepper(stepper) {}

const ProgramInfo &ParserStepper::getProgramInfo() const { return stepper.get().getProgramInfo(); }

ExecutionState &ParserStepper::getExecutionState() const {
    return stepper.get().getExecutionState();
}

ControlPlaneState &ParserStepper::getControlPlaneState() const {
    return stepper.get().getControlPlaneState();
}

bool ParserStepper::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Node %1% of type %2% not implemented in the core stepper.", node,
                      node->node_type_name());
}

/* =============================================================================================
 *  Visitor functions
 * ============================================================================================= */

bool ParserStepper::preorder(const IR::P4Parser *parser) {
    auto &executionState = getExecutionState();
    // Enter the parser's namespace.
    executionState.pushNamespace(parser);
    auto blockName = parser->getName().name;
    auto canonicalName = getProgramInfo().getCanonicalBlockName(blockName);
    const auto *parserParams = parser->getApplyParameters();
    const auto *archSpec = FlayTarget::getArchSpec();

    // Copy-in.
    for (size_t paramIdx = 0; paramIdx < parserParams->size(); ++paramIdx) {
        const auto *internalParam = parserParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        executionState.copyIn(FlayTarget::get(), internalParam, externalParamName);
    }

    // Declare local variables.
    for (const auto *decl : parser->parserLocals) {
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            executionState.declareVariable(FlayTarget::get(), *declVar);
        }
    }

    // Step into the start state.
    const auto *startState = parser->states.getDeclaration<IR::ParserState>("start");
    executionState.addParserId(startState->clone_id);
    startState->apply_visitor_preorder(*this);

    // Copy-out.
    for (size_t paramIdx = 0; paramIdx < parserParams->size(); ++paramIdx) {
        const auto *internalParam = parserParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        executionState.copyOut(internalParam, externalParamName);
    }
    executionState.popNamespace();
    return false;
}

void ParserStepper::processSelectExpression(const IR::SelectExpression *selectExpr) {
    auto &executionState = getExecutionState();
    auto &resolver = stepper.get().createExpressionResolver(getProgramInfo(), getExecutionState(),
                                                            getControlPlaneState());
    const auto *selectKeyExpr = resolver.computeResult(selectExpr->select);

    const IR::Expression *cond = IR::getBoolLiteral(false);
    std::vector<const IR::Expression *> notConds;
    std::vector<std::reference_wrapper<const ExecutionState>> accumulatedStates;
    for (const auto *selectCase : selectExpr->selectCases) {
        // The default label must be last. Always break here.
        // We handle the default case separately.
        if (selectCase->keyset->is<IR::DefaultExpression>()) {
            break;
        }
        // Actually execute the select expression.
        const auto *decl =
            getExecutionState().findDecl(selectCase->state)->checkedTo<IR::ParserState>();
        int declId = decl->clone_id;
        if (executionState.hasVisitedParserId(declId)) {
            P4C_UNIMPLEMENTED(
                "Parser state %1% was already visited. We currently do not support parser loops.",
                selectCase->state);
            continue;
        }
        const auto *selectCaseMatchExpr = resolver.computeResult(selectCase->keyset);
        auto &selectState = executionState.clone();
        selectState.addParserId(declId);
        const auto *matchCond = GenEq::equate(selectKeyExpr, selectCaseMatchExpr);
        cond = new IR::LOr(cond, matchCond);
        const auto *finalCond = cond;
        for (const auto *notCond : notConds) {
            finalCond = new IR::LAnd(notCond, finalCond);
        }
        notConds.push_back(new IR::LNot(cond));
        selectState.pushExecutionCondition(finalCond);
        auto subParserStepper = ParserStepper(
            FlayTarget::getStepper(getProgramInfo(), selectState, getControlPlaneState()));
        decl->apply(subParserStepper);
        // Save the state for  later merging.
        accumulatedStates.emplace_back(selectState);
    }

    // First, run the default label and get the state that would be covered in this case.
    for (const auto *selectCase : selectExpr->selectCases) {
        if (selectCase->keyset->is<IR::DefaultExpression>()) {
            const auto *decl =
                getExecutionState().findDecl(selectCase->state)->checkedTo<IR::ParserState>();
            decl->apply_visitor_preorder(*this);
            break;
        }
    }
    // After, merge all the accumulated state.
    for (auto accumulatedState : accumulatedStates) {
        executionState.merge(accumulatedState);
    }
}

bool ParserStepper::preorder(const IR::ParserState *parserState) {
    // Only bother looking up cases that are not accept or reject.
    if (parserState->name == IR::ParserState::accept) {
        return false;
    }
    if (parserState->name == IR::ParserState::reject) {
        return false;
    }

    auto &executionState = getExecutionState();
    // Enter the parser state's namespace.
    executionState.pushNamespace(parserState);
    for (const auto *declOrStmt : parserState->components) {
        declOrStmt->apply_visitor_preorder(stepper);
    }

    const auto *select = parserState->selectExpression;

    if (const auto *selectExpr = select->to<IR::SelectExpression>()) {
        processSelectExpression(selectExpr);
    } else if (const auto *pathExpression = select->to<IR::PathExpression>()) {
        executionState.popNamespace();
        // If we are referencing a parser state, step into the executionState.
        const auto *decl = executionState.findDecl(pathExpression)->checkedTo<IR::ParserState>();
        int declId = decl->clone_id;
        if (executionState.hasVisitedParserId(declId)) {
            P4C_UNIMPLEMENTED(
                "Parser state %1% was already visited. We currently do not support parser loops.",
                pathExpression);
        } else {
            executionState.addParserId(declId);
            decl->apply_visitor_preorder(*this);
        }
    } else {
        P4C_UNIMPLEMENTED("Select expression %1% not implemented for parser states.", select);
    }
    return false;
}

const std::vector<ParserStepper::ParserExitState> &ParserStepper::getParserExitStates() const {
    return parserExitStates;
}

void ParserStepper::addParserExitState(const ExecutionState &state) {
    parserExitStates.emplace_back(state);
}
}  // namespace P4Tools::Flay
