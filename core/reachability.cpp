#include "backends/p4tools/modules/flay/core/reachability.h"

#include "lib/error.h"

namespace P4Tools::Flay {

const IR::Expression *ReachabilityExpression::getCondition() const { return cond; }

void ReachabilityExpression::setCondition(const IR::Expression *cond) { this->cond = cond; }

std::optional<bool> ReachabilityExpression::getReachability() const {
    return reachabilityAssignment;
}

void ReachabilityExpression::setReachability(std::optional<bool> reachability) {
    reachabilityAssignment = reachability;
}

ReachabilityExpression::ReachabilityExpression(const IR::Expression *cond,
                                               std::optional<bool> reachabilityAssignment)
    : cond(cond), reachabilityAssignment(reachabilityAssignment) {}
ReachabilityExpression::ReachabilityExpression(const IR::Expression *cond)
    : cond(cond), reachabilityAssignment(std::nullopt) {}

bool ReachabilityMap::initializeReachabilityMapping(const IR::Node *node,
                                                    const IR::Expression *cond, bool addIfExist) {
    SymbolCollector collector;
    cond->apply(collector);
    const auto &collectedSymbols = collector.getCollectedSymbols();
    for (const auto &symbol : collectedSymbols) {
        symbolMap[symbol.get()].emplace(node);
    }

    auto result = emplace(node, std::vector<ReachabilityExpression>());
    if (!result.second && !addIfExist) {
        return false;
    }
    result.first->second.push_back(ReachabilityExpression(cond));
    return result.second;
}

void ReachabilityMap::mergeReachabilityMapping(const ReachabilityMap &otherMap) {
    for (const auto &rechabilityTuple : otherMap) {
        insert(rechabilityTuple);
    }
    for (const auto &symbol : otherMap.getSymbolMap()) {
        symbolMap[symbol.first.get()].insert(symbol.second.begin(), symbol.second.end());
    }
}

void ReachabilityMap::substitutePlaceholders(Transform &substitute) {
    for (auto &[node, reachabilityExpressionVector] : *this) {
        for (auto &reachabilityExpression : reachabilityExpressionVector) {
            reachabilityExpression.setCondition(
                reachabilityExpression.getCondition()->apply(substitute));
        }
    }
}
SymbolMap ReachabilityMap::getSymbolMap() const { return symbolMap; }

SolverReachabilityMap::SolverReachabilityMap(AbstractSolver &solver, const ReachabilityMap &map)
    : ReachabilityMap(map), symbolMap(map.getSymbolMap()), solver(solver) {}

std::optional<bool> SolverReachabilityMap::computeNodeReachability(
    const IR::Node *node, const std::vector<const Constraint *> &constraints) {
    auto vectorIt = find(node);
    if (vectorIt == end()) {
        ::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    bool hasChanged = false;
    for (auto &it : vectorIt->second) {
        const auto *reachabilityCondition = it.getCondition();
        auto reachabilityAssignment = it.getReachability();

        std::vector<const Constraint *> mergedConstraints(constraints);
        mergedConstraints.push_back(reachabilityCondition);
        auto solverResult = solver.get().checkSat(mergedConstraints);
        /// Solver returns unknown, better leave this alone.
        if (solverResult == std::nullopt) {
            ::warning("Solver returned unknown result for %1%.", node);
            hasChanged = reachabilityAssignment.has_value();
            continue;
        }

        /// There is no way to satisfy the condition. It is always false.
        if (!solverResult.value()) {
            it.setReachability(false);
            hasChanged = !reachabilityAssignment.has_value() || reachabilityAssignment.value();
            continue;
        }

        std::vector<const Constraint *> mergedConstraints1(constraints);
        mergedConstraints1.push_back(new IR::LNot(reachabilityCondition));
        auto solverResult1 = solver.get().checkSat(mergedConstraints1);
        /// Solver returns unknown, better leave this alone.
        if (solverResult1 == std::nullopt) {
            ::warning("Solver returned unknown result for %1%.", node);
            hasChanged = reachabilityAssignment.has_value();
            continue;
        }
        /// There is no way to falsify the condition. It is always true.
        if (!solverResult1.value()) {
            it.setReachability(true);
            hasChanged = !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
            continue;
        }

        if (solverResult.value() && solverResult1.value()) {
            it.setReachability(std::nullopt);
            hasChanged = reachabilityAssignment.has_value();
        }
    }

    return hasChanged;
}

std::optional<std::vector<const ReachabilityExpression *>>
SolverReachabilityMap::getReachabilityExpressions(const IR::Node *node) const {
    auto vectorIt = find(node);
    if (vectorIt != end()) {
        BUG_CHECK(!vectorIt->second.empty(), "Reachability vector for node %1% is empty.", node);
        std::vector<const ReachabilityExpression *> result;
        for (auto &it : vectorIt->second) {
            result.push_back(&it);
        }
        return result;
    }
    return std::nullopt;
}

bool SolverReachabilityMap::updateReachabilityAssignment(const IR::Node *node,
                                                         std::optional<bool> reachability) {
    /// FIXME: Do we want to keep this? I cannot find any usage.
    P4C_UNIMPLEMENTED("This is not implemented since changing to vector");
    // auto it = find(node);
    // if (it != end()) {
    //     it->second.setReachability(reachability);
    //     return true;
    // }
    return false;
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
        auto it = symbolMap.find(symbol);
        if (it != symbolMap.end()) {
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
