#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_Z3SOLVER_REACHABILITY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_Z3SOLVER_REACHABILITY_H_

#include <z3++.h>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/core/reachability.h"

namespace P4Tools::Flay {

/// A Z3ReachabilityExpression extends the ReachabilityExpression class with precomputed Z3
/// expressions. This significantly can improve performance for complex programs.
class Z3ReachabilityExpression : public ReachabilityExpression {
 private:
    /// The condition for the expression to be executable in Z3 form.
    z3::expr z3Condition;

 public:
    explicit Z3ReachabilityExpression(ReachabilityExpression reachabilityExpression,
                                      z3::expr z3Condition);

    /// @returns the precomputed Z3 condition.
    [[nodiscard]] const z3::expr &getZ3Condition() const;
};

class Z3SolverReachabilityMap
    : private std::map<const IR::Node *, std::set<Z3ReachabilityExpression *>, SourceIdCmp>,
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

    std::optional<std::set<const ReachabilityExpression *>> getReachabilityExpressions(
        const IR::Node *node) const override;

    std::optional<bool> recomputeReachability(
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeReachability(
        const SymbolSet &symbolSet,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeReachability(
        const NodeSet &targetNodes,
        const ControlPlaneConstraints &controlPlaneConstraints) override;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_Z3SOLVER_REACHABILITY_H_
