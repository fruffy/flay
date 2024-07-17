#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_Z3SOLVER_REACHABILITY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_Z3SOLVER_REACHABILITY_H_

#include <z3++.h>

#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"
#include "backends/p4tools/modules/flay/core/specialization/reachability_map.h"

namespace P4Tools::Flay {

/// A Z3ReachabilityExpression extends the ReachabilityExpression class with precomputed Z3
/// expressions. This significantly can improve performance for complex programs.
class Z3ReachabilityExpression : public ReachabilityExpression {
 private:
    /// The condition for the expression to be executable in Z3 form.
    z3::expr _z3Condition;

 public:
    explicit Z3ReachabilityExpression(ReachabilityExpression reachabilityExpression,
                                      z3::expr z3Condition);

    /// @returns the precomputed Z3 condition.
    [[nodiscard]] z3::expr &getZ3Condition();
};

class Z3SolverReachabilityMap
    : private std::map<const IR::Node *, Z3ReachabilityExpression *, SourceIdCmp>,
      public AbstractReachabilityMap {
 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// reachability map. This map can we used for incremental re-computation of reachability.
    SymbolMap _symbolMap;

    /// The Z3 solver used for incremental re-computation of reachability.
    Z3Solver _solver;

    /// Compute reachability for the node given the set of constraints.
    std::optional<bool> computeNodeReachability(const IR::Node *node,
                                                const z3::expr_vector &variables,
                                                const z3::expr_vector &variableAssignments);

 public:
    explicit Z3SolverReachabilityMap(const NodeAnnotationMap &map);

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

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_Z3SOLVER_REACHABILITY_H_
