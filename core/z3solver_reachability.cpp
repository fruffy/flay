#include "backends/p4tools/modules/flay/core/z3solver_reachability.h"

#include <z3++.h>

#include <cstdio>
#include <utility>

#include "backends/p4tools/common/lib/logging.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

Z3ReachabilityExpression::Z3ReachabilityExpression(ReachabilityExpression reachabilityExpression,
                                                   z3::expr z3Condition)
    : ReachabilityExpression(reachabilityExpression), z3Condition(std::move(z3Condition)) {}

const z3::expr &Z3ReachabilityExpression::getZ3Condition() const { return z3Condition; }

std::optional<bool> Z3SolverReachabilityMap::computeNodeReachability(const IR::Node *node) {
    auto it = find(node);
    if (it == end()) {
        ::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    bool hasChanged = false;
    auto *reachabilityExpression = it->second;
    const auto &reachabilityCondition = reachabilityExpression->getZ3Condition();
    auto reachabilityAssignment = reachabilityExpression->getReachability();
    {
        auto expressionVector = z3::expr_vector(_solver.mutableContext());
        expressionVector.push_back(reachabilityCondition);
        auto solverResult = _solver.checkSat(expressionVector);
        /// Solver returns unknown, better leave this alone.
        if (!solverResult.has_value()) {
            ::warning("Solver returned unknown result for %1%.", node);
            return reachabilityAssignment.has_value();
        }

        /// There is no way to satisfy the condition. It is always false.
        if (!solverResult.value()) {
            reachabilityExpression->setReachability(false);
            return !reachabilityAssignment.has_value() || reachabilityAssignment.value();
        }
    }
    {
        auto expressionVector = z3::expr_vector(_solver.mutableContext());
        expressionVector.push_back(!reachabilityCondition);
        auto solverResult = _solver.checkSat(expressionVector);
        /// Solver returns unknown, better leave this alone.
        if (!solverResult.has_value()) {
            ::warning("Solver returned unknown result for %1%.", node);
            return reachabilityAssignment.has_value();
        }
        /// There is no way to falsify the condition. It is always true.
        if (!solverResult.value()) {
            reachabilityExpression->setReachability(true);
            return !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
        }

        if (solverResult.value()) {
            reachabilityExpression->setReachability(std::nullopt);
            return reachabilityAssignment.has_value();
        }
    }
    return hasChanged;
}

Z3SolverReachabilityMap::Z3SolverReachabilityMap(const NodeAnnotationMap &map)
    : _symbolMap(map.reachabilitySymbolMap()) {
    Util::ScopedTimer timer("Precomputing Z3 Reachability");
    Z3Translator z3Translator(_solver);
    for (const auto &[node, reachabilityExpression] : map.reachabilityMap()) {
        (*this)[node] = new Z3ReachabilityExpression(
            reachabilityExpression,
            z3Translator.translate(reachabilityExpression.getCondition()).simplify());
    }
}

std::optional<bool> Z3SolverReachabilityMap::isNodeReachable(const IR::Node *node) const {
    auto it = find(node);
    if (it != end()) {
        const auto &reachabilityNode = it->second;
        // printFeature("flay_reachability_mapping", 4, "Reachability: %1% = %2%.", node,
        // reachabilityNode->getCondition());
        return reachabilityNode->getReachability();
    }
    ::warning(
        "Unable to find node %1% in the reachability map of this execution state. There might be "
        "issues with the source information.",
        node);
    return std::nullopt;
}

std::optional<bool> Z3SolverReachabilityMap::recomputeReachability(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    _solver.push();
    Z3Translator z3Translator(_solver);
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto &controlPlaneAssignments =
            controlPlaneConstraint.get().computeControlPlaneAssignments();
        for (const auto &constraint : controlPlaneAssignments) {
            _solver.asrt((z3::operator==(z3Translator.translate(&constraint.first.get()),
                                         z3Translator.translate(&constraint.second.get()))));
        }
    }
    auto result = _solver.checkSat();
    if (result == std::nullopt || !result.value()) {
        ::error("Unreachable constraints");
        _solver.pop();
        return std::nullopt;
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeReachability(pair.first);
        if (!result.has_value()) {
            _solver.pop();
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    _solver.pop();
    return hasChanged;
}

std::optional<bool> Z3SolverReachabilityMap::recomputeReachability(
    const SymbolSet &symbolSet, const ControlPlaneConstraints &controlPlaneConstraints) {
    NodeSet targetNodes;
    for (const auto &symbol : symbolSet) {
        auto it = _symbolMap.find(symbol);
        if (it != _symbolMap.end()) {
            for (const auto *node : it->second) {
                targetNodes.insert(node);
            }
        }
    }
    return recomputeReachability(targetNodes, controlPlaneConstraints);
}

std::optional<bool> Z3SolverReachabilityMap::recomputeReachability(
    const NodeSet &targetNodes, const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    _solver.push();
    Z3Translator z3Translator(_solver);
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto &controlPlaneAssignments =
            controlPlaneConstraint.get().computeControlPlaneAssignments();
        for (const auto &constraint : controlPlaneAssignments) {
            _solver.asrt((z3::operator==(z3Translator.translate(&constraint.first.get()),
                                         z3Translator.translate(&constraint.second.get()))));
        }
    }
    auto result = _solver.checkSat();
    if (result == std::nullopt || !result.value()) {
        ::error("Unreachable constraints");
        _solver.pop();
        return std::nullopt;
    }

    bool hasChanged = false;
    for (const auto *node : targetNodes) {
        auto result = computeNodeReachability(node);
        if (!result.has_value()) {
            _solver.pop();
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    _solver.pop();
    return hasChanged;
}

}  // namespace P4Tools::Flay
