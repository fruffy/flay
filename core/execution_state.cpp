#include "backends/p4tools/modules/flay/core/execution_state.h"

#include <utility>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/collapse_mux.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

namespace P4Tools::Flay {

bool SourceIdCmp::operator()(const IR::Node *s1, const IR::Node *s2) const {
    return s1->srcInfo < s2->srcInfo;
}

ExecutionState::ExecutionState(const IR::P4Program *program)
    : AbstractExecutionState(program), executionCondition(IR::getBoolLiteral(true)) {}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

const IR::Expression *ExecutionState::convertToStructLikeExpression(
    const IR::Expression *parent) const {
    // TODO: We are losing information about validity here. How do we also record the validity?
    if (auto ts = parent->type->to<IR::Type_StructLike>()) {
        IR::Vector<IR::Expression> components;
        for (auto structField : ts->fields) {
            auto fieldName = structField->name;
            auto fieldType = structField->type;
            auto ref = new IR::Member(fieldType, parent, fieldName);
            if (fieldType->is<IR::Type_StructLike>() || fieldType->to<IR::Type_Stack>()) {
                components.push_back(convertToStructLikeExpression(ref));
            } else {
                components.push_back(get(ref));
            }
        }
        return new IR::ListExpression(parent->type, components);
    } else if (auto ts = parent->type->to<IR::Type_Stack>()) {
        IR::Vector<IR::Expression> components;
        for (size_t idx = 0; idx < ts->getSize(); idx++) {
            auto ref = new IR::ArrayIndex(ts->elementType, parent, new IR::Constant(idx));
            if (ts->elementType->is<IR::Type_StructLike>() ||
                ts->elementType->to<IR::Type_Stack>()) {
                components.push_back(convertToStructLikeExpression(ref));
            } else {
                components.push_back(get(ref));
            }
        }
        return new IR::ListExpression(parent->type, components);
    }
    P4C_UNIMPLEMENTED("Unsupported struct-like type %1% for member %2%",
                      parent->type->node_type_name(), parent);
}

const IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    // In some cases, we may reference a complex expression. Convert it to a struct expression.
    if (var->type->is<IR::Type_StructLike>() || var->type->to<IR::Type_Stack>()) {
        return convertToStructLikeExpression(var);
    }
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

    for (const auto &rechabilityTuple : mergeState.getReachabilityMap()) {
        reachabilityMap.insert(rechabilityTuple);
    }
}

const ReachabilityMap &ExecutionState::getReachabilityMap() const { return reachabilityMap; }

const IR ::Expression *ExecutionState::getReachabilityCondition(const IR::Node *node,
                                                                bool checked) const {
    auto it = reachabilityMap.find(node);
    if (it != reachabilityMap.end()) {
        return it->second;
    }
    BUG_CHECK(!checked, "Unable to find node %1% with source info %2% in the reachability map.",
              node, node->srcInfo.toPositionString());
    return nullptr;
}

void ExecutionState::addReachabilityMapping(const IR::Node *node, const IR::Expression *cond) {
    reachabilityMap[node] = new IR::LAnd(getExecutionCondition(), cond);
}

/* =========================================================================================
 *  Constructors
 * ========================================================================================= */

ExecutionState &ExecutionState::create(const IR::P4Program *program) {
    return *new ExecutionState(program);
}

ExecutionState &ExecutionState::clone() const { return *new ExecutionState(*this); }

}  // namespace P4Tools::Flay
