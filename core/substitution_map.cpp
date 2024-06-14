#include "backends/p4tools/modules/flay/core/substitution_map.h"

#include <z3++.h>

#include <optional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "lib/error.h"

namespace P4Tools::Flay {

Z3SolverSubstitutionMap::Z3SolverSubstitutionMap(Z3Solver &solver, const NodeAnnotationMap &map)
    : _symbolMap(map.expressionSymbolMap()), _solver(solver) {
    Z3Translator translator(_solver);
    for (auto &[node, substitutionExpression] : map.expressionMap()) {
        auto *z3SubstitutionExpression = new Z3SubstitutionExpression(
            substitutionExpression->condition(), substitutionExpression->originalExpression(),
            translator.translate(substitutionExpression->originalExpression()).simplify());
        emplace(node, z3SubstitutionExpression);
    }
}

std::optional<bool> Z3SolverSubstitutionMap::computeNodeSubstitution(
    const IR::Expression *expression, const z3::expr_vector &variables,
    const z3::expr_vector &variableAssignments) {
    auto it = find(expression);
    if (it == end()) {
        ::error("Substitution mapping for node %1% does not exist.", expression);
        return std::nullopt;
    }

    auto original = it->second->originalZ3Expression();
    auto newExpr = original.substitute(variables, variableAssignments).simplify();
    auto previousSubstitution = it->second->substitution();
    if (newExpr.is_numeral()) {
        const auto *newSubstitution = IR::Constant::get(
            expression->type,
            big_int(newExpr.get_decimal_string(expression->type->width_bits()).c_str()),
            expression->getSourceInfo());
        it->second->setSubstitution(newSubstitution);
        return !previousSubstitution.has_value() ||
               !previousSubstitution.value()->equiv(*newSubstitution);
    }
    if (previousSubstitution.has_value()) {
        it->second->unsetSubstitution();
        return true;
    }

    return false;
}

std::optional<const IR::Literal *> Z3SolverSubstitutionMap::isExpressionConstant(
    const IR::Expression *expression) const {
    auto it = find(expression);
    if (it != end()) {
        return it->second->substitution();
    }
    ::warning(
        "Unable to find node %1% in the expression map of this execution state. There might be "
        "issues with the source information.",
        expression);
    return std::nullopt;
}

std::optional<bool> Z3SolverSubstitutionMap::recomputeSubstitution(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    Z3Translator translator(_solver);
    auto variables = z3::expr_vector(_solver.get().mutableContext());
    auto variableAssignments = z3::expr_vector(_solver.get().mutableContext());
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto &entityConstraints =
            controlPlaneConstraint.get().computeControlPlaneAssignments();
        for (const auto &constraint : entityConstraints) {
            variables.push_back(translator.translate(&constraint.first.get()));
            variableAssignments.push_back(translator.translate(&constraint.second.get()));
        }
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeSubstitution(pair.first, variables, variableAssignments);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

std::optional<bool> Z3SolverSubstitutionMap::recomputeSubstitution(
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

std::optional<bool> Z3SolverSubstitutionMap::recomputeSubstitution(
    const ExpressionSet &targetExpressions,
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    Z3Translator translator(_solver);
    auto variables = z3::expr_vector(_solver.get().mutableContext());
    auto variableAssignments = z3::expr_vector(_solver.get().mutableContext());
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto &entityConstraints =
            controlPlaneConstraint.get().computeControlPlaneAssignments();
        for (const auto &constraint : entityConstraints) {
            variables.push_back(translator.translate(&constraint.first.get()));
            variableAssignments.push_back(translator.translate(&constraint.second.get()));
        }
    }
    bool hasChanged = false;
    for (const auto *node : targetExpressions) {
        auto result = computeNodeSubstitution(node, variables, variableAssignments);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

}  // namespace P4Tools::Flay
