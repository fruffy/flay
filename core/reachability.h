#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_

#include <map>
#include <optional>

#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// Utility function to compare IR nodes in a set. We use their source info.
struct SourceIdCmp {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const;
};

/// A reachability expression encodes the condition need for an expression to be executable and the
/// current calculated result assigned to that expression. If the assignment is std::nullopt, the
/// expression can be executed. If the assignment is true, the expression is always executed. If it
/// is false, never.
struct ReachabilityExpression {
 private:
    const IR::Expression *cond;
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
class ReachabilityMap : private std::map<const IR::Node *, ReachabilityExpression, SourceIdCmp> {
 private:
    /// Compute reachability for the node given the set of constraints.
    std::optional<bool> computeNodeReachability(const IR::Node *node, AbstractSolver &solver,
                                                const std::vector<const Constraint *> &constraints);

 public:
    /// @returns the reachability expression for the given node.
    /// @returns std::nullopt if no expression can be found.
    [[nodiscard]] std::optional<ReachabilityExpression> getReachabilityExpression(
        const IR::Node *node) const;

    /// Updates the reachability assignment for the given node.
    bool updateReachabilityAssignment(const IR::Node *node, std::optional<bool> reachability);

    /// Compute reachability for all nodes in the map using the provided control plane constraints.
    std::optional<bool> recomputeReachability(
        AbstractSolver &solver, const ControlPlaneConstraints &initialControlPlaneConstraints);

    /// Initialize the reachability mapping for the given node.
    /// @returns false if the node is already mapped.
    bool initializeReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    /// Merge an other reachability map into this reachability map.
    void mergeReachabilityMapping(const ReachabilityMap &otherMap);

    /// Substitute all placeholders in the reachability map and update each condition.
    void substitutePlaceholders(Transform &substitute);
};

}  // namespace P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_ */
