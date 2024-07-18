#include "backends/p4tools/modules/flay/core/specialization/reachability_map.h"

#include "backends/p4tools/modules/flay/core/control_plane/substitute_variable.h"
#include "backends/p4tools/modules/flay/core/lib/simplify_expression.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

IRReachabilityMap::IRReachabilityMap(const NodeAnnotationMap &map)
    : _symbolMap(map.reachabilitySymbolMap()) {
    for (auto &pair : map.reachabilityMap()) {
        emplace(pair.first, pair.second);
    }
}

std::optional<bool> IRReachabilityMap::computeNodeReachability(
    const IR::Node *node, const ControlPlaneAssignmentSet &controlPlaneAssignments) {
    auto it = find(node);
    if (it == end()) {
        ::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    auto *reachabilityExpression = it->second;
    const auto *reachabilityCondition = reachabilityExpression->getCondition();
    reachabilityCondition =
        reachabilityCondition->apply(SubstituteSymbolicVariable(controlPlaneAssignments));
    reachabilityCondition = SimplifyExpression::simplify(reachabilityCondition);
    auto reachabilityAssignment = reachabilityExpression->getReachability();
    if (const auto *boolLiteral = reachabilityCondition->to<IR::BoolLiteral>()) {
        if (boolLiteral->value) {
            reachabilityExpression->setReachability(true);
            return !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
        }
        reachabilityExpression->setReachability(false);
        return !reachabilityAssignment.has_value() || reachabilityAssignment.value();
    }
    if (reachabilityAssignment.has_value()) {
        reachabilityExpression->setReachability(std::nullopt);
        return true;
    }
    return false;
}

std::optional<bool> IRReachabilityMap::isNodeReachable(const IR::Node *node) const {
    auto it = find(node);
    if (it != end()) {
        return it->second->getReachability();
    }
    ::warning(
        "Unable to find node %1% in the reachability map of this execution state. There might be "
        "issues with the source information.",
        node);
    return std::nullopt;
}

std::optional<bool> IRReachabilityMap::recomputeReachability(
    const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    ControlPlaneAssignmentSet totalControlPlaneAssignments;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto &controlPlaneAssignments =
            controlPlaneConstraint.get().computeControlPlaneAssignments();
        totalControlPlaneAssignments.insert(controlPlaneAssignments.begin(),
                                            controlPlaneAssignments.end());
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeReachability(pair.first, totalControlPlaneAssignments);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

std::optional<bool> IRReachabilityMap::recomputeReachability(
    const SymbolSet &symbolSet, const ControlPlaneConstraints &controlPlaneConstraints) {
    Util::ScopedTimer timer("IRReachabilityMap::recomputeReachability with symbol set");
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

std::optional<bool> IRReachabilityMap::recomputeReachability(
    const NodeSet &targetNodes, const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    ControlPlaneAssignmentSet totalControlPlaneAssignments;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        const auto &controlPlaneAssignments =
            controlPlaneConstraint.get().computeControlPlaneAssignments();
        totalControlPlaneAssignments.insert(controlPlaneAssignments.begin(),
                                            controlPlaneAssignments.end());
    }

    bool hasChanged = false;
    for (const auto *node : targetNodes) {
        auto result = computeNodeReachability(node, totalControlPlaneAssignments);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

}  // namespace P4Tools::Flay
