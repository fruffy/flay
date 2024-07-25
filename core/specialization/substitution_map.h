#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SUBSTITUTION_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SUBSTITUTION_MAP_H_

#include <optional>

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"

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

class IrSubstitutionMap : private SubstitutionMap, public AbstractSubstitutionMap {
 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// substitution map. This map can we used for incremental re-computation of substitution.
    SymbolMap _symbolMap;

    /// Compute substitution for the node given the set of constraints.
    std::optional<bool> computeNodeSubstitution(
        const IR::Expression *expression, const ControlPlaneAssignmentSet &controlPlaneAssignments);

 public:
    explicit IrSubstitutionMap(const NodeAnnotationMap &map);

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

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SUBSTITUTION_MAP_H_ */
