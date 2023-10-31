#include "backends/p4tools/modules/flay/core/stepper.h"

#include <cstddef>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/parser_stepper.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
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
    // Delegate execution to the parser stepper.
    ParserStepper parserStepper(*this);
    parser->apply(parserStepper);
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

    // Copy-in.
    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        executionState.copyIn(FlayTarget::get(), internalParam, externalParamName);
    }

    // Declare local variables.
    for (const auto *decl : control->controlLocals) {
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            executionState.declareVariable(FlayTarget::get(), *declVar);
        }
    }

    // Step into the actual control body.
    control->body->apply_visitor_preorder(*this);

    // Copy-out.
    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        executionState.copyOut(internalParam, externalParamName);
    }
    executionState.popNamespace();
    return false;
}

void assignStruct(ExecutionState &executionState, const IR::Expression *left,
                  const IR::Expression *right, const IR::Type_StructLike *ts) {
    if (const auto *structExpr = right->to<IR::StructExpression>()) {
        std::vector<IR::StateVariable> flatTargetValids;
        auto flatTargetFields = executionState.getFlatFields(left, ts, &flatTargetValids);
        auto flatStructFields = IR::flattenStructExpression(structExpr);
        auto flatTargetSize = flatTargetFields.size();
        auto flatStructSize = flatStructFields.size();
        BUG_CHECK(flatTargetSize == flatStructSize,
                  "The size of target fields (%1%) and the size of source fields (%2%) are "
                  "different.",
                  flatTargetSize, flatStructSize);
        for (const auto &fieldTargetValid : flatTargetValids) {
            executionState.set(fieldTargetValid, IR::getBoolLiteral(true));
        }
        // First, complete the assignments for the data structure.
        for (size_t idx = 0; idx < flatTargetFields.size(); ++idx) {
            const auto &flatTargetRef = flatTargetFields[idx];
            const auto *flatStructField = flatStructFields[idx];
            executionState.set(flatTargetRef, flatStructField);
        }
    } else if (right->is<IR::PathExpression>() || right->is<IR::Member>() ||
               right->is<IR::ArrayIndex>()) {
        executionState.setStructLike(left, right);
    } else {
        P4C_UNIMPLEMENTED("Unsupported assignment rval %1% of type %2%", right,
                          right->node_type_name());
    }
}

bool FlayStepper::preorder(const IR::AssignmentStatement *assign) {
    const auto *right = assign->right;
    const auto *left = assign->left;
    auto &executionState = getExecutionState();
    auto &programInfo = getProgramInfo();

    const auto *assignType = executionState.resolveType(left->type);
    if (const auto *ts = assignType->to<IR::Type_StructLike>()) {
        // TODO: Support validity of header assignments and complex struct headers.
        if (right->is<IR::MethodCallExpression>()) {
            // Resolve the rval of the assignment statement.
            auto &resolver = createExpressionResolver(programInfo, executionState);
            right = resolver.computeResult(right);
        }
        assignStruct(executionState, left, right, ts);
        return false;
    } else if (const auto *ts = assignType->to<IR::Type_Stack>()) {
        if (right->is<IR::MethodCallExpression>()) {
            // Resolve the rval of the assignment statement.
            auto &resolver = createExpressionResolver(programInfo, executionState);
            right = resolver.computeResult(right);
        }
        for (size_t idx = 0; idx < ts->getSize(); idx++) {
            auto ref = new IR::ArrayIndex(ts->elementType, left, new IR::Constant(idx));
            auto structType = ts->elementType->to<IR::Type_StructLike>();
            assignStruct(executionState, ref,
                         new IR::ArrayIndex(ts->elementType, right, new IR::Constant(idx)),
                         structType);
        }
        return false;
    }
    // Resolve the rval of the assignment statement.
    auto &resolver = createExpressionResolver(programInfo, executionState);
    right = resolver.computeResult(right);

    if (assignType->is<IR::Type_Base>()) {
        executionState.set(ToolsVariables::convertReference(left), right);
    } else {
        P4C_UNIMPLEMENTED("Unsupported assignment type %1% of type %2% from %3%", assignType,
                          assignType->node_type_name(), right);
    }

    return false;
}

bool FlayStepper::preorder(const IR::EmptyStatement * /*emptyStatement*/) {
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

bool FlayStepper::preorder(const IR::IfStatement *ifStatement) {
    auto &executionState = getExecutionState();

    const auto *cond = ifStatement->condition;
    auto &resolver = createExpressionResolver(getProgramInfo(), executionState);
    cond = resolver.computeResult(cond);

    // Add the node to the reachability map.
    executionState.addReachabilityMapping(ifStatement, cond);

    // Execute the case where the condition is true.
    auto &trueState = executionState.clone();
    trueState.pushExecutionCondition(cond);
    auto &trueStepper = FlayTarget::getStepper(programInfo, trueState);
    ifStatement->ifTrue->apply(trueStepper);

    // Execute the alternative.
    if (ifStatement->ifFalse != nullptr) {
        ifStatement->ifFalse->apply_visitor_preorder(*this);
    }
    // Merge.
    executionState.merge(trueState);
    return false;
}

bool FlayStepper::preorder(const IR::SwitchStatement *switchStatement) {
    // Check whether this is a table switch-case first.
    auto tableMode = false;
    if (const auto *member = switchStatement->expression->to<IR::Member>()) {
        if (member->expr->is<IR::MethodCallExpression>() &&
            member->member.name == IR::Type_Table::action_run) {
            tableMode = true;
        }
    }

    auto &executionState = getExecutionState();
    // Resolve the switch match expression.
    auto &resolver = createExpressionResolver(getProgramInfo(), executionState);
    const auto *switchExpr = resolver.computeResult(switchStatement->expression);

    const IR::Expression *cond = IR::getBoolLiteral(false);
    std::vector<const IR::Statement *> accumulatedStatements;
    std::vector<const IR::Expression *> notConds;
    std::vector<std::reference_wrapper<const ExecutionState>> accumulatedStates;
    for (const auto *switchCase : switchStatement->cases) {
        // The default label must be last. Always break here.
        // We handle the default case separately.
        if (switchCase->label->is<IR::DefaultExpression>()) {
            break;
        }
        const auto *switchCaseLabel = switchCase->label;
        // In table mode, we are actually comparing string expressions.
        if (tableMode) {
            const auto *path = switchCaseLabel->checkedTo<IR::PathExpression>();
            switchCaseLabel = new IR::StringLiteral(path->path->name);
        }
        cond = new IR::LOr(cond, new IR::Equ(switchExpr, switchCaseLabel));
        // Nothing to do with this statement. Fall through to the next case.
        if (switchCase->statement == nullptr) {
            continue;
        }
        // We fall through, so add the statements to execute to a list.
        accumulatedStatements.push_back(switchCase->statement);

        // If the statement is a block, we do not fall through and terminate execution.
        if (switchCase->statement->is<IR::BlockStatement>()) {
            // If any of the values in the match list hits, execute the switch case block.
            auto &caseState = executionState.clone();
            // The final condition is the accumulated label condition and NOT other conditions that
            // have previously matched.
            const auto *finalCond = cond;
            for (const auto *notCond : notConds) {
                finalCond = new IR::LAnd(notCond, finalCond);
            }
            notConds.push_back(new IR::LNot(cond));
            cond = IR::getBoolLiteral(false);
            caseState.pushExecutionCondition(finalCond);
            // Execute the state with the accumulated statements.
            auto &switchStepper = FlayTarget::getStepper(programInfo, caseState);
            for (const auto *statement : accumulatedStatements) {
                statement->apply(switchStepper);
            }
            accumulatedStatements.clear();
            // Save the state for  later merging.
            accumulatedStates.emplace_back(caseState);
        }
    }

    // First, run the default label and get the state that would be covered in this case.
    for (const auto *switchCase : switchStatement->cases) {
        if (switchCase->label->is<IR::DefaultExpression>()) {
            switchCase->statement->apply_visitor_preorder(*this);
            break;
        }
    }
    // After, merge all the accumulated state.
    for (auto accumulatedState : accumulatedStates) {
        executionState.merge(accumulatedState);
    }

    return false;
}

bool FlayStepper::preorder(const IR::MethodCallStatement *callStatement) {
    auto &resolver = createExpressionResolver(getProgramInfo(), getExecutionState());
    callStatement->methodCall->apply(resolver);
    return false;
}

}  // namespace P4Tools::Flay
