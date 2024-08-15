#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_REACHABILITY_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_REACHABILITY_MAP_H_

#include <optional>

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"

namespace P4::P4Tools::Flay {

class AbstractReachabilityMap {
 public:
    AbstractReachabilityMap(const AbstractReachabilityMap &) = default;
    AbstractReachabilityMap(AbstractReachabilityMap &&) = delete;
    AbstractReachabilityMap &operator=(const AbstractReachabilityMap &) = default;
    AbstractReachabilityMap &operator=(AbstractReachabilityMap &&) = delete;

    AbstractReachabilityMap() = default;
    virtual ~AbstractReachabilityMap() = default;

    /// Compute reachability for all nodes in the map using the provided control plane constraints.
    std::optional<bool> virtual recomputeReachability(
        const ControlPlaneConstraints &controlPlaneConstraints) = 0;

    /// Recompute reachability for all nodes which depend on any of the variables in the given
    /// symbol set.
    std::optional<bool> virtual recomputeReachability(
        const SymbolSet &symbolSet, const ControlPlaneConstraints &controlPlaneConstraints) = 0;

    /// Recompute reachability for selected nodes in the map using the provided control plane
    /// constraints.
    std::optional<bool> virtual recomputeReachability(
        const NodeSet &targetNodes, const ControlPlaneConstraints &controlPlaneConstraints) = 0;

    /// @return false when the node is never reachable,
    /// true when the node is always reachable, and std::nullopt if the node is sometimes reachable
    /// or the node could not be found.
    virtual std::optional<bool> isNodeReachable(const IR::Node *node) const = 0;
};

class IRReachabilityMap : private ReachabilityMap, public AbstractReachabilityMap {
 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// reachability map. This map can we used for incremental re-computation of reachability.
    SymbolMap _symbolMap;

    /// Compute reachability for the node given the set of constraints.
    std::optional<bool> computeNodeReachability(
        const IR::Node *node, const ControlPlaneAssignmentSet &controlPlaneAssignments);

 public:
    explicit IRReachabilityMap(const NodeAnnotationMap &map);

    std::optional<bool> recomputeReachability(
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeReachability(
        const SymbolSet &symbolSet,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeReachability(
        const NodeSet &targetNodes,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> isNodeReachable(const IR::Node *node) const override;
};

}  // namespace P4::P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_REACHABILITY_MAP_H_ */
