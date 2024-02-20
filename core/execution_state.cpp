#include "backends/p4tools/modules/flay/core/execution_state.h"

#include <utility>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/simplify_expression.h"
#include "backends/p4tools/modules/flay/core/substitute_placeholders.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

ExecutionState::ExecutionState(const IR::P4Program *program)
    : AbstractExecutionState(program), executionCondition(IR::getBoolLiteral(true)) {}

const IR::PathExpression ExecutionState::PLACEHOLDER_LABEL = IR::PathExpression("*placeholder");

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

const IR::Expression *ExecutionState::createSymbolicExpression(const IR::Type *inputType,
                                                               cstring label) const {
    const auto *resolvedType = resolveType(inputType);
    if (const auto *structType = resolvedType->to<IR::Type_StructLike>()) {
        IR::IndexedVector<IR::NamedExpression> fields;
        for (const auto *field : structType->fields) {
            auto fieldLabel = label + "_" + field->name;
            fields.push_back(new IR::NamedExpression(
                field->name, createSymbolicExpression(field->type, fieldLabel)));
        }
        if (structType->is<IR::Type_Header>()) {
            cstring labelId = label + "_" + ToolsVariables::VALID;
            const auto *validity =
                ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), labelId);
            // TODO: We keep the struct type anonymous because we do not know it.
            return new IR::HeaderExpression(structType, nullptr, fields, validity);
        }
        // TODO: We keep the struct type anonymous because we do not know it.
        return new IR::StructExpression(structType, nullptr, fields);
    }
    if (const auto *stackType = resolvedType->to<IR::Type_Stack>()) {
        IR::Vector<IR::Expression> fields;
        for (size_t idx = 0; idx < stackType->getSize(); ++idx) {
            auto fieldLabel = label + "[" + std::to_string(idx) + "]";
            fields.push_back(createSymbolicExpression(stackType->elementType, fieldLabel));
        }
        return new IR::HeaderStackExpression(fields, inputType);
    }
    if (resolvedType->is<IR::Type_Base>()) {
        return ToolsVariables::getSymbolicVariable(resolvedType, label);
    }
    P4C_UNIMPLEMENTED("Requesting a symbolic expression for %1% of type %2%", inputType,
                      inputType->node_type_name());
}

const IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    const auto *varType = resolveType(var->type);
    // In some cases, we may reference a complex expression. Convert it to a struct expression.
    if (varType->is<IR::Type_StructLike>() || varType->is<IR::Type_Stack>()) {
        return convertToComplexExpression(var);
    }
    return env.get(var);
}

void ExecutionState::set(const IR::StateVariable &var, const IR::Expression *value) {
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
    executionCondition = SimplifyExpression::simplify(new IR::LAnd(executionCondition, cond));
}

void ExecutionState::merge(const ExecutionState &mergeState) {
    const auto *cond = mergeState.getExecutionCondition();
    cond = SimplifyExpression::simplify(cond);
    const auto &mergeEnv = mergeState.getSymbolicEnv();
    reachabilityMap.mergeReachabilityMapping(mergeState.getReachabilityMap());

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
        // Do not merge any variable that did not exist previously.
        if (exists(ref)) {
            const auto *currentExpr = get(ref);
            // Only merge when the current and the merged expression are different.
            if (!currentExpr->equiv(*mergeExpr)) {
                set(envTuple.first,
                    SimplifyExpression::produceSimplifiedMux(cond, mergeExpr, currentExpr));
            }
        }
    }
}

const ReachabilityMap &ExecutionState::getReachabilityMap() const { return reachabilityMap; }

void ExecutionState::addReachabilityMapping(const IR::Node *node, const IR::Expression *cond) {
    // TODO: Think about better handling of these types of errors?
    if (!node->getSourceInfo().isValid()) {
        return;
    }

    if (!reachabilityMap.initializeReachabilityMapping(
            node, new IR::LAnd(getExecutionCondition(), cond))) {
        // Throw a fatal error if we try to add a duplicate mapping.
        // This can affect the correctness of the entire mapping.
        BUG("Reachability mapping for node %1% already exists. Every mapping must be uniquely "
            "identifiable.",
            node);
    }
}

void ExecutionState::setPlaceholderValue(cstring label, const IR::Expression *value) {
    const auto *placeholderVar = new IR::Member(value->type, &PLACEHOLDER_LABEL, label);
    set(placeholderVar, value);
}

std::optional<const IR::Expression *> ExecutionState::getPlaceholderValue(
    const IR::Placeholder &placeholder) const {
    const auto *placeholderVar =
        new IR::Member(placeholder.type, &PLACEHOLDER_LABEL, placeholder.label);
    if (exists(placeholderVar)) {
        return get(placeholderVar);
    }
    return std::nullopt;
}

void ExecutionState::substitutePlaceholders() {
    Util::ScopedTimer timer("Placeholder Substitution");
    auto substitute = SubstitutePlaceHolders(*this);
    reachabilityMap.substitutePlaceholders(substitute);
}

/* =========================================================================================
 *  Constructors
 * ========================================================================================= */

ExecutionState &ExecutionState::create(const IR::P4Program *program) {
    return *new ExecutionState(program);
}

ExecutionState &ExecutionState::clone() const { return *new ExecutionState(*this); }

}  // namespace P4Tools::Flay
