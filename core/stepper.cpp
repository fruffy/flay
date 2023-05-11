#include "backends/p4tools/modules/flay/core/stepper.h"

#include <cstddef>
#include <string>
#include <vector>

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

FlayStepper::FlayStepper(const ProgramInfo &programInfo, ExecutionState &executionState)
    : programInfo(programInfo), executionState(executionState) {}

const ProgramInfo &FlayStepper::getProgramInfo() const { return programInfo.get(); }

ExecutionState &FlayStepper::getExecutionState() const { return executionState.get(); }

bool FlayStepper::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Node %1% of type %2% not implemented in the core stepper.", node,
                      node->node_type_name());
}

/* =============================================================================================
 *  Visitor functions
 * ============================================================================================= */

bool FlayStepper::preorder(const IR::P4Parser *parser) {
    auto &executionState = getExecutionState();
    // Enter the parser's namespace.
    executionState.pushNamespace(parser);
    auto blockName = parser->getName().name;
    auto canonicalName = getProgramInfo().getCanonicalBlockName(blockName);
    const auto *parserParams = parser->getApplyParameters();
    const auto *archSpec = FlayTarget::getArchSpec();
    for (size_t paramIdx = 0; paramIdx < parserParams->size(); ++paramIdx) {
        const auto *internalParam = parserParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        StateUtils::copyIn(executionState, getProgramInfo(), internalParam, externalParamName);
    }
    const auto *startState = parser->states.getDeclaration<IR::ParserState>("start");
    startState->apply_visitor_preorder(*this);

    for (size_t paramIdx = 0; paramIdx < parserParams->size(); ++paramIdx) {
        const auto *internalParam = parserParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        StateUtils::copyOut(executionState, internalParam, externalParamName);
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::P4Control *control) {
    auto &executionState = getExecutionState();
    // Enter the control's namespace.
    executionState.pushNamespace(control);
    auto blockName = control->getName().name;
    auto canonicalName = getProgramInfo().getCanonicalBlockName(blockName);
    const auto *controlParams = control->getApplyParameters();
    const auto *archSpec = FlayTarget::getArchSpec();
    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        StateUtils::copyIn(executionState, getProgramInfo(), internalParam, externalParamName);
    }
    for (const auto *decl : control->controlLocals) {
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            StateUtils::declareVariable(getProgramInfo(), getExecutionState(), *declVar);
        }
    }

    control->body->apply_visitor_preorder(*this);

    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        StateUtils::copyOut(executionState, internalParam, externalParamName);
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::ParserState *parserState) {
    auto &executionState = getExecutionState();
    // Enter the parser state's namespace.
    executionState.pushNamespace(parserState);
    for (const auto *declOrStmt : parserState->components) {
        declOrStmt->apply_visitor_preorder(*this);
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::AssignmentStatement *assign) {
    const auto *right = assign->right;
    const auto *left = assign->left;
    auto &executionState = getExecutionState();

    const auto *assignType = executionState.resolveType(left->type);
    if (const auto *ts = assignType->to<IR::Type_StructLike>()) {
        if (const auto *structExpr = right->to<IR::StructExpression>()) {
            std::vector<IR::StateVariable> flatTargetValids;
            auto flatTargetFields =
                StateUtils::getFlatFields(executionState, left, ts, &flatTargetValids);
            auto flatStructFields = StateUtils::getFlatStructFields(structExpr);
            BUG_CHECK(
                flatTargetFields.size() == flatStructFields.size(),
                "The list of target fields and the list of source fields have different sizes.");
            for (const auto &fieldTargetValid : flatTargetValids) {
                executionState.set(fieldTargetValid, IR::getBoolLiteral(true));
            }
            // First, complete the assignments for the data structure.
            for (size_t idx = 0; idx < flatTargetFields.size(); ++idx) {
                const auto &flatTargetRef = flatTargetFields[idx];
                const auto *flatStructField = flatStructFields[idx];
                executionState.set(flatTargetRef, flatStructField);
            }
        } else if (right->is<IR::PathExpression>() || right->is<IR::Member>()) {
            StateUtils::setStructLike(executionState, left, right);
        } else {
            P4C_UNIMPLEMENTED("Unsupported assignment rval %1%", right->node_type_name());
        }
        return false;
    }
    // Resolve the rval of the assignment statement.
    auto &resolver = createExpressionResolver(getProgramInfo(), getExecutionState());
    right->apply(resolver);
    right = resolver.getResult();

    if (assignType->is<IR::Type_Base>()) {
        auto leftRef = StateUtils::convertReference(left);
        executionState.set(leftRef, right);
    } else {
        P4C_UNIMPLEMENTED("Unsupported assignment type %1%", assignType->node_type_name());
    }

    return false;
}

bool FlayStepper::preorder(const IR::EmptyStatement * /*emptyStmt*/) {
    // This is a no-op
    return false;
}

bool FlayStepper::preorder(const IR::BlockStatement *block) {
    auto &executionState = getExecutionState();

    // Enter the block's namespace.
    executionState.pushNamespace(block);
    for (const auto *declOrStmt : block->components) {
        declOrStmt->apply_visitor_preorder(*this);
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::IfStatement *ifStmt) {
    auto &executionState = getExecutionState();

    const auto *cond = ifStmt->condition;
    auto &resolver = createExpressionResolver(getProgramInfo(), getExecutionState());
    cond->apply(resolver);
    cond = resolver.getResult();

    auto &trueState = executionState.clone();
    auto &trueStepper = FlayTarget::getStepper(programInfo, trueState);
    ifStmt->ifTrue->apply(trueStepper);

    if (ifStmt->ifFalse != nullptr) {
        ifStmt->ifFalse->apply_visitor_preorder(*this);
    }
    executionState.merge(trueState.getSymbolicEnv(), cond);
    return false;
}

bool FlayStepper::preorder(const IR::MethodCallStatement *stmt) {
    auto &resolver = createExpressionResolver(getProgramInfo(), getExecutionState());
    stmt->methodCall->apply(resolver);
    return false;
}

}  // namespace P4Tools::Flay
