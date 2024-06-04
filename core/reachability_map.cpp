#include "backends/p4tools/modules/flay/core/reachability_map.h"

#include "lib/error.h"

namespace P4Tools::Flay {

SolverReachabilityMap::SolverReachabilityMap(AbstractSolver &solver, const NodeAnnotationMap &map)
    : _symbolMap(map.symbolMap()), _solver(solver) {
    for (auto &pair : map.reachabilityMap()) {
        emplace(pair.first, pair.second);
    }
}

std::optional<bool> SolverReachabilityMap::computeNodeReachability(
    const IR::Node *node, const std::vector<const Constraint *> &constraints) {
    auto vectorIt = find(node);
    if (vectorIt == end()) {
        ::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    bool hasChanged = false;
    for (const auto &it : vectorIt->second) {
        const auto *reachabilityCondition = it->getCondition();
        auto reachabilityAssignment = it->getReachability();

        std::vector<const Constraint *> mergedConstraints(constraints);
        mergedConstraints.push_back(reachabilityCondition);
        auto solverResult = _solver.get().checkSat(mergedConstraints);
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

        std::vector<const Constraint *> mergedConstraints1(constraints);
        mergedConstraints1.push_back(new IR::LNot(reachabilityCondition));
        auto solverResult1 = _solver.get().checkSat(mergedConstraints1);
        /// Solver returns unknown, better leave this alone.
        if (solverResult1 == std::nullopt) {
            ::warning("Solver returned unknown result for %1%.", node);
            hasChanged = reachabilityAssignment.has_value();
            continue;
        }
        /// There is no way to falsify the condition. It is always true.
        if (!solverResult1.value()) {
            it->setReachability(true);
            hasChanged = !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
            continue;
        }

        if (solverResult.value() && solverResult1.value()) {
            it->setReachability(std::nullopt);
            hasChanged = reachabilityAssignment.has_value();
        }
    }

    return hasChanged;
}

std::optional<bool> SolverReachabilityMap::isNodeReachable(const IR::Node *node) const {
    auto vectorIt = find(node);
    if (vectorIt != end()) {
        BUG_CHECK(!vectorIt->second.empty(), "Reachability vector for node %1% is empty.", node);
        for (const auto &reachabilityNode : vectorIt->second) {
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

std::optional<bool> SolverReachabilityMap::recomputeReachability(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    std::vector<const Constraint *> constraints;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        constraints.push_back(controlPlaneConstraint.get().computeControlPlaneConstraint());
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeReachability(pair.first, constraints);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

std::optional<bool> SolverReachabilityMap::recomputeReachability(
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

std::optional<bool> SolverReachabilityMap::recomputeReachability(
    const NodeSet &targetNodes, const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    std::vector<const Constraint *> constraints;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        constraints.push_back(controlPlaneConstraint.get().computeControlPlaneConstraint());
    }
    bool hasChanged = false;
    for (const auto *node : targetNodes) {
        auto result = computeNodeReachability(node, constraints);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

}  // namespace P4Tools::Flay
