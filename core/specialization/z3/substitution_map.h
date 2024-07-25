#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_Z3_SUBSTITUTION_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_Z3_SUBSTITUTION_MAP_H_

#include <z3++.h>

#include <functional>
#include <optional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"
#include "backends/p4tools/modules/flay/core/specialization/substitution_map.h"

namespace P4Tools::Flay {

/**************************************************************************************************
Z3SubstitutionExpression
**************************************************************************************************/

struct Z3SubstitutionExpression : public SubstitutionExpression {
 private:
    /// The original expression translated to Z3.
    z3::expr _originalZ3Expression;

 public:
    Z3SubstitutionExpression(const IR::Expression *condition,
                             const IR::Expression *originalExpression,
                             z3::expr originalZ3Expression);

    /// @returns the original expression translated to Z3 form.
    [[nodiscard]] const z3::expr &originalZ3Expression() const;
};

/// The expression map but using Z3 expressions instead of IR expressions.
using Z3ExpressionMap = std::map<const IR::Expression *, Z3SubstitutionExpression *, SourceIdCmp>;

class Z3SolverSubstitutionMap : private Z3ExpressionMap, public AbstractSubstitutionMap {
 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// substitution map. This map can we used for incremental re-computation of substitution.
    SymbolMap _symbolMap;

    /// Compute substitution for the node given the set of constraints.
    std::optional<bool> computeNodeSubstitution(const IR::Expression *expression,
                                                const Z3ControlPlaneAssignmentSet &assignmentSet);

 public:
    explicit Z3SolverSubstitutionMap(const NodeAnnotationMap &map);

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

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_Z3_SUBSTITUTION_MAP_H_ */
