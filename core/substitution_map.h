#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTION_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTION_MAP_H_

#include <z3++.h>

#include <functional>
#include <optional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/core/node_map.h"
#include "ir/solver.h"

namespace P4Tools::Flay {

class AbstractSubstitutionMap {
 public:
    AbstractSubstitutionMap(const AbstractSubstitutionMap &) = default;
    AbstractSubstitutionMap(AbstractSubstitutionMap &&) = delete;
    AbstractSubstitutionMap &operator=(const AbstractSubstitutionMap &) = default;
    AbstractSubstitutionMap &operator=(AbstractSubstitutionMap &&) = delete;

    AbstractSubstitutionMap() = default;
    virtual ~AbstractSubstitutionMap() = default;

    /// Compute substitution for all nodes in the map using the provided control plane constraints.
    std::optional<bool> virtual recomputeSubstitution(
        const ControlPlaneConstraints &controlPlaneConstraints) = 0;

    /// Recompute substitution for all nodes which depend on any of the variables in the given
    /// symbol set.
    std::optional<bool> virtual recomputeSubstitution(
        const SymbolSet &symbolSet, const ControlPlaneConstraints &controlPlaneConstraints) = 0;

    /// Recompute substitution for selected nodes in the map using the provided control plane
    /// constraints.
    std::optional<bool> virtual recomputeSubstitution(
        const ExpressionSet &targetExpressions,
        const ControlPlaneConstraints &controlPlaneConstraints) = 0;

    /// @return true if the node can be replace with a constant, false otherwise
    virtual std::optional<const IR::Literal *> isExpressionConstant(
        const IR::Expression *expression) const = 0;
};

class Z3SolverSubstitutionMap : private Z3ExpressionMap, public AbstractSubstitutionMap {
 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// substitution map. This map can we used for incremental re-computation of substitution.
    SymbolMap _symbolMap;

    /// The solver used to compute substitution.
    std::reference_wrapper<Z3Solver> _solver;

    /// Compute substitution for the node given the set of constraints.
    std::optional<bool> computeNodeSubstitution(const IR::Expression *expression,
                                                const z3::expr_vector &variables,
                                                const z3::expr_vector &variableAssignments);

 public:
    explicit Z3SolverSubstitutionMap(Z3Solver &solver, const NodeAnnotationMap &map);

    std::optional<bool> recomputeSubstitution(
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeSubstitution(
        const SymbolSet &symbolSet,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<bool> recomputeSubstitution(
        const ExpressionSet &targetExpressions,
        const ControlPlaneConstraints &controlPlaneConstraints) override;

    std::optional<const IR::Literal *> isExpressionConstant(
        const IR::Expression *expression) const override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTION_MAP_H_ */
