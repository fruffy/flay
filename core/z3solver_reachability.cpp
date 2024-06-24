#include "backends/p4tools/modules/flay/core/z3solver_reachability.h"

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
            _solver.push();
            _solver.asrt(reachabilityCondition);
            auto solverResult = _solver.checkSat();
            _solver.pop();
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
            _solver.push();
            _solver.asrt(!reachabilityCondition);
            auto solverResult = _solver.checkSat();
            _solver.pop();
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

Z3SolverReachabilityMap::Z3SolverReachabilityMap(const NodeAnnotationMap &map)
    : _symbolMap(map.reachabilitySymbolMap()) {
    Util::ScopedTimer timer("Precomputing Z3 Reachability");
    Z3Translator z3Translator(_solver);
    for (const auto &[node, reachabilityExpressionVector] : map.reachabilityMap()) {
        std::set<Z3ReachabilityExpression *> result;
        for (const auto &reachabilityExpression : reachabilityExpressionVector) {
            result.emplace(new Z3ReachabilityExpression(
                *reachabilityExpression,
                z3Translator.translate(reachabilityExpression->getCondition()).simplify()));
        }
        (*this)[node].insert(result.begin(), result.end());
    }
}

std::optional<bool> Z3SolverReachabilityMap::isNodeReachable(const IR::Node *node) const {
    auto vectorIt = find(node);
    if (vectorIt != end()) {
        BUG_CHECK(!vectorIt->second.empty(), "Reachability vector for node %1% is empty.", node);
        for (const auto &reachabilityNode : vectorIt->second) {
            // printFeature("flay_reachability_mapping", 4, "Reachability: %1% = %2%.", node,
            // reachabilityNode->getCondition());
            auto reachability = reachabilityNode->getReachability();
            if (!reachability.has_value()) {
                return std::nullopt;
            }
            if (reachability.value()) {
                return true;
            }
        }
        return false;
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
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto *constraint = controlPlaneConstraint.get().computeControlPlaneConstraint();
        _solver.asrt(constraint);
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
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto *constraint = controlPlaneConstraint.get().computeControlPlaneConstraint();
        _solver.asrt(constraint);
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
