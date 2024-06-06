#include "backends/p4tools/modules/flay/core/substitution_map.h"

#include <optional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "lib/error.h"

namespace P4Tools::Flay {

SolverSubstitutionMap::SolverSubstitutionMap(Z3Solver &solver, const NodeAnnotationMap &map)
    : _symbolMap(map.expressionSymbolMap()), _solver(solver) {
    for (auto &pair : map.expressionMap()) {
        emplace(pair.first, pair.second);
    }
}

std::optional<bool> SolverSubstitutionMap::computeNodeSubstitution(
    const IR::Expression *expression, const std::vector<const Constraint *> & /*constraints*/) {
    auto it = find(expression);
    if (it == end()) {
        ::error("Substitution mapping for node %1% does not exist.", expression);
        return std::nullopt;
    }
    return false;
}

std::optional<const IR::Literal *> SolverSubstitutionMap::isExpressionConstant(
    const IR::Expression *expression) const {
    auto it = find(expression);
    if (it != end()) {
        return expression->is<IR::Literal>() ? std::optional(it->second->to<IR::Literal>())
                                             : std::nullopt;
    }
    ::warning(
        "Unable to find node %1% in the expression map of this execution state. There might be "
        "issues with the source information.",
        expression);
    return std::nullopt;
}

std::optional<bool> SolverSubstitutionMap::recomputeSubstitution(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    std::vector<const Constraint *> constraints;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        constraints.push_back(controlPlaneConstraint.get().computeControlPlaneConstraint());
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeSubstitution(pair.first, constraints);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

std::optional<bool> SolverSubstitutionMap::recomputeSubstitution(
    const SymbolSet &symbolSet, const ControlPlaneConstraints &controlPlaneConstraints) {
    ExpressionSet targetExpressions;
    for (const auto &symbol : symbolSet) {
        auto it = _symbolMap.find(symbol);
        if (it != _symbolMap.end()) {
            for (const auto *node : it->second) {
                targetExpressions.insert(node->checkedTo<IR::Expression>());
            }
        }
    }
    return recomputeSubstitution(targetExpressions, controlPlaneConstraints);
}

std::optional<bool> SolverSubstitutionMap::recomputeSubstitution(
    const ExpressionSet &targetExpressions,
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    std::vector<const Constraint *> constraints;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        constraints.push_back(controlPlaneConstraint.get().computeControlPlaneConstraint());
    }
    bool hasChanged = false;
    for (const auto *node : targetExpressions) {
        auto result = computeNodeSubstitution(node, constraints);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

}  // namespace P4Tools::Flay
