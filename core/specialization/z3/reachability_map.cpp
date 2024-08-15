#include "backends/p4tools/modules/flay/core/specialization/z3/reachability_map.h"

#include <z3++.h>

#include <cstdio>
#include <utility>

#include "lib/timer.h"

namespace P4::P4Tools::Flay {

Z3ReachabilityExpression::Z3ReachabilityExpression(ReachabilityExpression reachabilityExpression,
                                                   z3::expr z3Condition)
    : ReachabilityExpression(reachabilityExpression), _z3Condition(std::move(z3Condition)) {}

z3::expr &Z3ReachabilityExpression::getZ3Condition() { return _z3Condition; }

std::optional<bool> Z3SolverReachabilityMap::computeNodeReachability(
    const IR::Node *node, const Z3ControlPlaneAssignmentSet &assignments) {
    auto it = find(node);
    if (it == end()) {
        ::P4::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    auto *reachabilityExpression = it->second;
    auto &reachabilityCondition = reachabilityExpression->getZ3Condition();
    auto newExpr = assignments.substitute(reachabilityCondition).simplify();
    auto reachabilityAssignment = reachabilityExpression->getReachability();
    auto declKind = newExpr.decl().decl_kind();
    if (declKind == Z3_decl_kind::Z3_OP_FALSE || declKind == Z3_decl_kind::Z3_OP_TRUE) {
        if (newExpr.bool_value() == Z3_lbool::Z3_L_TRUE) {
            reachabilityExpression->setReachability(true);
            return !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
        }
        if (newExpr.bool_value() == Z3_lbool::Z3_L_FALSE) {
            reachabilityExpression->setReachability(false);
            return !reachabilityAssignment.has_value() || reachabilityAssignment.value();
        }
    }
    if (reachabilityAssignment.has_value()) {
        reachabilityExpression->setReachability(std::nullopt);
        return true;
    }
    return false;
}

Z3SolverReachabilityMap::Z3SolverReachabilityMap(const NodeAnnotationMap &map)
    : _symbolMap(map.reachabilitySymbolMap()) {
    Util::ScopedTimer timer("Precomputing Z3 Reachability");
    for (const auto &[node, reachabilityExpression] : map.reachabilityMap()) {
        (*this)[node] = new Z3ReachabilityExpression(
            *reachabilityExpression,
            Z3Cache::set(reachabilityExpression->getCondition()).simplify());
        // printInfo("Computing reachability for %1%:\t%2%", node,
        //           reachabilityExpression->getCondition());
        // printInfo("##############");
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
    ::P4::warning(
        "Unable to find node %1% in the reachability map of this execution state. There might be "
        "issues with the source information.",
        node);
    return std::nullopt;
}

std::optional<bool> Z3SolverReachabilityMap::recomputeReachability(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    Z3ControlPlaneAssignmentSet assignmentSet;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        assignmentSet.merge(controlPlaneConstraint.get().computeZ3ControlPlaneAssignments());
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeReachability(pair.first, assignmentSet);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
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
    Z3ControlPlaneAssignmentSet assignmentSet;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        assignmentSet.merge(controlPlaneConstraint.get().computeZ3ControlPlaneAssignments());
    }

    bool hasChanged = false;
    for (const auto *node : targetNodes) {
        auto result = computeNodeReachability(node, assignmentSet);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

}  // namespace P4::P4Tools::Flay
