#include "backends/p4tools/modules/flay/core/specialization/z3/substitution_map.h"

#include <z3++.h>

#include <optional>

#include "lib/error.h"
#include "lib/timer.h"

namespace P4::P4Tools::Flay {

/**************************************************************************************************
Z3SubstitutionExpression
**************************************************************************************************/

Z3SubstitutionExpression::Z3SubstitutionExpression(const IR::Expression *condition,
                                                   const IR::Expression *originalExpression,
                                                   z3::expr originalZ3Expression)
    : SubstitutionExpression(condition, originalExpression),
      _originalZ3Expression(std::move(originalZ3Expression)) {}

const z3::expr &Z3SubstitutionExpression::originalZ3Expression() const {
    return _originalZ3Expression;
}

/**************************************************************************************************
Z3SolverSubstitutionMap
**************************************************************************************************/

Z3SolverSubstitutionMap::Z3SolverSubstitutionMap(const NodeAnnotationMap &map)
    : _symbolMap(map.expressionSymbolMap()) {
    Util::ScopedTimer timer("Precomputing Z3 Substitution Map");
    for (auto &[node, substitutionExpression] : map.substitutionMap()) {
        auto *z3SubstitutionExpression = new Z3SubstitutionExpression(
            substitutionExpression->condition(), substitutionExpression->originalExpression(),
            Z3Cache::set(substitutionExpression->originalExpression()).simplify());
        emplace(node, z3SubstitutionExpression);
    }
}

std::optional<bool> Z3SolverSubstitutionMap::computeNodeSubstitution(
    const IR::Expression *expression, const Z3ControlPlaneAssignmentSet &assignmentSet) {
    auto it = find(expression);
    if (it == end()) {
        error("Substitution mapping for node %1% does not exist.", expression);
        return std::nullopt;
    }

    auto original = it->second->originalZ3Expression();
    auto newExpr = assignmentSet.substitute(original).simplify();
    auto previousSubstitution = it->second->substitution();
    auto declKind = newExpr.decl().decl_kind();
    if (declKind == Z3_decl_kind::Z3_OP_FALSE || declKind == Z3_decl_kind::Z3_OP_TRUE) {
        const auto *newSubstitution =
            IR::BoolLiteral::get(newExpr.is_true(), expression->getSourceInfo());
        it->second->setSubstitution(newSubstitution);
        return !previousSubstitution.has_value() ||
               !previousSubstitution.value()->equiv(*newSubstitution);
    }
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
    ::P4::warning(
        "Unable to find node %1% in the expression map of this execution state. There might be "
        "issues with the source information.",
        expression);
    return std::nullopt;
}

std::optional<bool> Z3SolverSubstitutionMap::recomputeSubstitution(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    Z3ControlPlaneAssignmentSet assignmentSet;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        assignmentSet.merge(controlPlaneConstraint.get().computeZ3ControlPlaneAssignments());
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeSubstitution(pair.first, assignmentSet);
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
    Z3ControlPlaneAssignmentSet assignmentSet;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        assignmentSet.merge(controlPlaneConstraint.get().computeZ3ControlPlaneAssignments());
    }

    bool hasChanged = false;
    for (const auto *node : targetExpressions) {
        auto result = computeNodeSubstitution(node, assignmentSet);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

}  // namespace P4::P4Tools::Flay
