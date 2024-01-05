#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_Z3SOLVER_REACHABILITY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_Z3SOLVER_REACHABILITY_H_

#include <z3++.h>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/core/reachability.h"

namespace P4Tools::Flay {

class Z3ReachabilityExpression : public ReachabilityExpression {
 private:
    /// The condition for the expression to be executable in Z3 form.
    z3::expr z3Condition;

 public:
    explicit Z3ReachabilityExpression(ReachabilityExpression reachabilityExpression,
                                      z3::expr z3Condition);

    [[nodiscard]] const z3::expr &getZ3Condition() const;
};

class Z3SolverReachabilityMap
    : private std::map<const IR::Node *, Z3ReachabilityExpression, SourceIdCmp>,
      public AbstractReachabilityMap {
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// reachability map. This map can we used for incremental re-computation of reachability.
    SymbolMap symbolMap;

 private:
    Z3Solver solver;

    /// Compute reachability for the node given the set of constraints.
    std::optional<bool> computeNodeReachability(const IR::Node *node);

 public:
    explicit Z3SolverReachabilityMap(const ReachabilityMap &map);

    /// @returns the reachability expression for the given node.
    /// @returns std::nullopt if no expression can be found.
    std::optional<const ReachabilityExpression *> getReachabilityExpression(
        const IR::Node *node) const override;

    /// Updates the reachability assignment for the given node.
    bool updateReachabilityAssignment(const IR::Node *node,
                                      std::optional<bool> reachability) override;

    /// Compute reachability for all nodes in the map using the provided control plane constraints.
    std::optional<bool> recomputeReachability(
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    /// Recompute reachability for all nodes which depend on any of the variables in the given
    /// symbol set.
    std::optional<bool> recomputeReachability(
        const SymbolSet &symbolSet,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    /// Recompute reachability for selected nodes in the map using the provided control plane
    /// constraints.
    std::optional<bool> recomputeReachability(
        const NodeSet &targetNodes,
        const ControlPlaneConstraints &controlPlaneConstraints) override;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_Z3SOLVER_REACHABILITY_H_
