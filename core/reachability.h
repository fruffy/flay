#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_

#include <map>
#include <optional>

#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// A reachability expression encodes the condition need for an expression to be executable and the
/// current calculated result assigned to that expression. If the assignment is std::nullopt, the
/// expression can be executed. If the assignment is true, the expression is always executed. If it
/// is false, never.
struct ReachabilityExpression {
 private:
    /// The conditions for the expression to be executable.
    const IR::Expression *cond;

    /// The current assigned value to `cond`. If the value is std::nullopt, the result is ambiguous.
    /// Otherwise, cond is considered either true or false and thus always or never true.
    std::optional<bool> reachabilityAssignment;

 public:
    explicit ReachabilityExpression(const IR::Expression *cond);

    ReachabilityExpression(const IR::Expression *cond, std::optional<bool> reachabilityAssignment);

    ReachabilityExpression(const ReachabilityExpression &) = default;
    ReachabilityExpression(ReachabilityExpression &&) = default;
    ReachabilityExpression &operator=(const ReachabilityExpression &) = default;
    ReachabilityExpression &operator=(ReachabilityExpression &&) = default;
    ~ReachabilityExpression() = default;

    /// Assign a concrete reachability value to the expression.
    void setReachability(std::optional<bool> reachability);

    /// Update the reachability expression
    void setCondition(const IR::Expression *cond);

    /// @returns the current reachability value.
    [[nodiscard]] std::optional<bool> getReachability() const;

    /// @returns the reachability expression.
    [[nodiscard]] const IR::Expression *getCondition() const;
};

/// A mapping of P4C-IR nodes to their associated reachability expressions.
class ReachabilityMap : protected std::map<const IR::Node *, ReachabilityExpression, SourceIdCmp> {
    friend class Z3SolverReachabilityMap;

 private:
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// reachability map. This map can we used for incremental re-computation of reachability.
    SymbolMap symbolMap;

 public:
    /// Initialize the reachability mapping for the given node.
    /// @returns false if the node is already mapped.
    bool initializeReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    /// Merge an other reachability map into this reachability map.
    void mergeReachabilityMapping(const ReachabilityMap &otherMap);

    /// Substitute all placeholders in the reachability map and update each condition.
    void substitutePlaceholders(Transform &substitute);

    /// @returns the symbol map accumulated in the map.
    [[nodiscard]] SymbolMap getSymbolMap() const;
};

class AbstractReachabilityMap {
 public:
    AbstractReachabilityMap(const AbstractReachabilityMap &) = default;
    AbstractReachabilityMap(AbstractReachabilityMap &&) = delete;
    AbstractReachabilityMap &operator=(const AbstractReachabilityMap &) = default;
    AbstractReachabilityMap &operator=(AbstractReachabilityMap &&) = delete;

    AbstractReachabilityMap() = default;
    virtual ~AbstractReachabilityMap() = default;

    /// @returns the reachability expression for the given node.
    /// @returns std::nullopt if no expression can be found.
    [[nodiscard]] virtual std::optional<const ReachabilityExpression *> getReachabilityExpression(
        const IR::Node *node) const = 0;

    /// Updates the reachability assignment for the given node.
    bool virtual updateReachabilityAssignment(const IR::Node *node,
                                              std::optional<bool> reachability) = 0;

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
};

class SolverReachabilityMap : private ReachabilityMap, public AbstractReachabilityMap {
    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// reachability map. This map can we used for incremental re-computation of reachability.
    SymbolMap symbolMap;

 private:
    AbstractSolver &solver;

    /// Compute reachability for the node given the set of constraints.
    std::optional<bool> computeNodeReachability(const IR::Node *node,
                                                const std::vector<const Constraint *> &constraints);

 public:
    explicit SolverReachabilityMap(AbstractSolver &solver, const ReachabilityMap &map);

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
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_ */
