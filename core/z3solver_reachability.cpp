#include "backends/p4tools/modules/flay/core/z3solver_reachability.h"

#include <utility>

#include "lib/timer.h"

namespace P4Tools::Flay {

Z3ReachabilityExpression::Z3ReachabilityExpression(ReachabilityExpression reachabilityExpression,
                                                   z3::expr z3Condition)
    : ReachabilityExpression(reachabilityExpression), z3Condition(std::move(z3Condition)) {}

const z3::expr &Z3ReachabilityExpression::getZ3Condition() const { return z3Condition; }

std::optional<bool> Z3SolverReachabilityMap::computeNodeReachability(const IR::Node *node) {
    auto vectorIt = find(node);
    if (vectorIt == end()) {
        ::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    bool hasChanged = false;
    for (const auto &it : vectorIt->second) {
        const auto &reachabilityCondition = it->getZ3Condition();
        auto reachabilityAssignment = it->getReachability();
        {
            solver.push();
            solver.asrt(reachabilityCondition);
            auto solverResult = solver.checkSat();
            solver.pop();
            /// Solver returns unknown, better leave this alone.
            if (solverResult == std::nullopt) {
                ::warning("Solver returned unknown result for %1%.", node);
                hasChanged = reachabilityAssignment.has_value();
                continue;
            }

            /// There is no way to satisfy the condition. It is always false.
            if (!solverResult.value()) {
                it->setReachability(false);
                hasChanged = !reachabilityAssignment.has_value() || reachabilityAssignment.value();
                continue;
            }
        }
        {
            solver.push();
            solver.asrt(!reachabilityCondition);
            auto solverResult = solver.checkSat();
            solver.pop();
            /// Solver returns unknown, better leave this alone.
            if (solverResult == std::nullopt) {
                ::warning("Solver returned unknown result for %1%.", node);
                hasChanged = reachabilityAssignment.has_value();
                continue;
            }
            /// There is no way to falsify the condition. It is always true.
            if (!solverResult.value()) {
                it->setReachability(true);
                hasChanged = !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
                continue;
            }

            if (solverResult.value() && solverResult.value()) {
                it->setReachability(std::nullopt);
                hasChanged = reachabilityAssignment.has_value();
            }
        }
    }
    return hasChanged;
}

Z3SolverReachabilityMap::Z3SolverReachabilityMap(const ReachabilityMap &map)
    : symbolMap(map.getSymbolMap()) {
    Util::ScopedTimer timer("Precomputing Z3 Reachability");
    Z3Translator z3Translator(solver);
    for (const auto &[node, reachabilityExpressionVector] : map) {
        std::set<Z3ReachabilityExpression *> result;
        for (const auto &reachabilityExpression : reachabilityExpressionVector) {
            reachabilityExpression->getCondition()->apply(z3Translator);
            result.emplace(
                new Z3ReachabilityExpression(*reachabilityExpression, z3Translator.getResult()));
        }
        (*this)[node].insert(result.begin(), result.end());
    }
}

std::optional<std::set<const ReachabilityExpression *>>
Z3SolverReachabilityMap::getReachabilityExpressions(const IR::Node *node) const {
    auto vectorIt = find(node);
    if (vectorIt != end()) {
        BUG_CHECK(!vectorIt->second.empty(), "Reachability vector for node %1% is empty.", node);
        std::set<const ReachabilityExpression *> result;
        for (const auto &it : vectorIt->second) {
            result.emplace(it);
        }
        return result;
    }
    return std::nullopt;
}

std::optional<bool> Z3SolverReachabilityMap::recomputeReachability(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    solver.push();
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto *constraint = controlPlaneConstraint.get().computeControlPlaneConstraint();
        solver.asrt(constraint);
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeReachability(pair.first);
        if (!result.has_value()) {
            solver.pop();
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    solver.pop();
    return hasChanged;
}

std::optional<bool> Z3SolverReachabilityMap::recomputeReachability(
    const SymbolSet &symbolSet, const ControlPlaneConstraints &controlPlaneConstraints) {
    NodeSet targetNodes;
    for (const auto &symbol : symbolSet) {
        auto it = symbolMap.find(symbol);
        if (it != symbolMap.end()) {
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
    solver.push();
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto *constraint = controlPlaneConstraint.get().computeControlPlaneConstraint();
        solver.asrt(constraint);
    }
    bool hasChanged = false;
    for (const auto *node : targetNodes) {
        auto result = computeNodeReachability(node);
        if (!result.has_value()) {
            solver.pop();
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    solver.pop();
    return hasChanged;
}

}  // namespace P4Tools::Flay
