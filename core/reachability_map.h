#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_MAP_H_

#include <functional>
#include <optional>

#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/core/node_map.h"
#include "ir/solver.h"

namespace P4Tools::Flay {

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

class SolverReachabilityMap : private ReachabilityMap, public AbstractReachabilityMap {
 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// reachability map. This map can we used for incremental re-computation of reachability.
    SymbolMap _symbolMap;

    /// The solver used to compute reachability.
    std::reference_wrapper<AbstractSolver> _solver;

    std::map<cstring, const IR::Expression *> _tableKeyConfigurations;

    /// Compute reachability for the node given the set of constraints.
    std::optional<bool> computeNodeReachability(const IR::Node *node,
                                                const std::vector<const Constraint *> &constraints);

 public:
    explicit SolverReachabilityMap(AbstractSolver &solver, const NodeAnnotationMap &map);

    std::optional<bool> recomputeReachability(
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeReachability(
        const SymbolSet &symbolSet,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeReachability(
        const NodeSet &targetNodes,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> isNodeReachable(const IR::Node *node) const override;

    bool addTableKeyConfiguration(cstring tableControlPlaneName, const IR::Expression *key) {
        _tableKeyConfigurations[tableControlPlaneName] = key;
        return true;
    }
    [[nodiscard]] const std::map<cstring, const IR::Expression *> &getTableKeyConfigurations()
        const {
        return _tableKeyConfigurations;
    }
    [[nodiscard]] std::optional<const IR::Expression *> getTableKeyConfigurations(
        cstring tableControlPlaneName) const {
        auto it = _tableKeyConfigurations.find(tableControlPlaneName);
        if (it == _tableKeyConfigurations.end()) {
            return std::nullopt;
        }
        return it->second;
    }
};

}  // namespace P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_MAP_H_ */
