#include "backends/p4tools/modules/flay/core/stepper.h"

#include <cstddef>
#include <string>
#include <vector>

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/parser_stepper.h"
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
        StateUtils::copyIn(executionState, getProgramInfo(), internalParam, externalParamName);
    }

    // Declare local variables.
    for (const auto *decl : control->controlLocals) {
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            StateUtils::declareVariable(getProgramInfo(), getExecutionState(), *declVar);
        }
    }

    // Step into the actual control body.
    control->body->apply_visitor_preorder(*this);

    // Copy-out.
    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        StateUtils::copyOut(executionState, internalParam, externalParamName);
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
    right = resolver.computeResult(right);

    if (assignType->is<IR::Type_Base>()) {
        auto leftRef = StateUtils::convertReference(left);
        executionState.set(leftRef, right);
    } else {
        P4C_UNIMPLEMENTED("Unsupported assignment type %1%", assignType->node_type_name());
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
    auto &resolver = createExpressionResolver(getProgramInfo(), getExecutionState());
    cond = resolver.computeResult(cond);

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

    // Resolve the switch match expression.
    auto &resolver = createExpressionResolver(getProgramInfo(), getExecutionState());
    const auto *switchExpr = resolver.computeResult(switchStatement->expression);

    auto &executionState = getExecutionState();

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
