#include "backends/p4tools/modules/flay/core/execution_state.h"

#include <utility>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/collapse_mux.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools::Flay {

ExecutionState::ExecutionState(const IR::P4Program *program)
    : AbstractExecutionState(program), executionCondition(IR::getBoolLiteral(true)) {}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

const IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    // TODO: This is a convoluted (and expensive?) check because struct members are not directly
    // associated with a header. We should be using runtime objects instead of flat assignments.
    const auto *expr = env.get(var);
    if (const auto *member = var->to<IR::Member>()) {
        if (member->expr->type->is<IR::Type_Header>() && member->member != ToolsVariables::VALID) {
            // If we are setting the member of a header, we need to check whether the
            // header is valid.
            // If the header is invalid, the get returns a tainted expression.
            // The member could have any value.
            auto validity = ToolsVariables::getHeaderValidity(member->expr);
            const auto *validVar = env.get(validity);
            if (const auto *validBool = validVar->to<IR::BoolLiteral>()) {
                if (validBool->value) {
                    return expr;
                }
                return IR::getDefaultValue(expr->type, expr->getSourceInfo(), true);
            }
            return new IR::Mux(expr->type, validVar, expr,
                               IR::getDefaultValue(expr->type, {}, true));
        }
    }
    return expr;
}

void ExecutionState::set(const IR::StateVariable &var, const IR::Expression *value) {
    // Small optimization. Do not nest Mux that are the same.
    if (value->is<IR::Mux>()) {
        value = value->apply(CollapseMux());
    }
    env.set(var, value);
}

void ExecutionState::addParserId(int parserId) { visitedParserIds.emplace(parserId); }

bool ExecutionState::hasVisitedParserId(int parserId) const {
    return visitedParserIds.find(parserId) != visitedParserIds.end();
}

/* =============================================================================================
 *  State merging and execution conditions.
 * ============================================================================================= */

const IR::Expression *ExecutionState::getExecutionCondition() const { return executionCondition; }

void ExecutionState::pushExecutionCondition(const IR::Expression *cond) {
    // cond = P4::optimizeExpression(new IR::LAnd(executionCondition, cond));
    executionCondition = P4::optimizeExpression(new IR::LAnd(executionCondition, cond));
}

void ExecutionState::merge(const ExecutionState &mergeState) {
    const auto *cond = mergeState.getExecutionCondition();
    const auto &mergeEnv = mergeState.getSymbolicEnv();
    if (const auto *boolExpr = cond->to<IR::BoolLiteral>()) {
        // If the condition is false, do nothing. If it is true, set all the values.
        if (boolExpr->value) {
            for (const auto &envTuple : mergeEnv.getInternalMap()) {
                auto ref = envTuple.first;
                if (exists(ref)) {
                    set(ref, envTuple.second);
                }
            }
        }
        return;
    }
    for (const auto &envTuple : mergeEnv.getInternalMap()) {
        auto ref = envTuple.first;
        const auto *mergeExpr = envTuple.second;
        if (exists(ref)) {
            const auto *currentExpr = get(ref);
            auto *mergedExpr = new IR::Mux(currentExpr->type, cond, mergeExpr, currentExpr);
            set(envTuple.first, mergedExpr);
        }
    }
}

/* =========================================================================================
 *  Constructors
 * ========================================================================================= */

ExecutionState &ExecutionState::create(const IR::P4Program *program) {
    return *new ExecutionState(program);
}

ExecutionState &ExecutionState::clone() const { return *new ExecutionState(*this); }

}  // namespace P4Tools::Flay
