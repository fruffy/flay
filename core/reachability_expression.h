#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_EXPRESSION_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_EXPRESSION_H_

#include <map>
#include <optional>

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "ir/ir.h"

namespace P4Tools::Flay {

/// A reachability expression encodes the condition need for an expression to be executable and the
/// current calculated result assigned to that expression. If the assignment is std::nullopt, the
/// expression can be executed. If the assignment is true, the expression is always executed. If it
/// is false, never.
struct ReachabilityExpression {
 private:
    /// The conditions for the expression to be executable.
    const IR::Expression *_cond;

    /// The current assigned value to `cond`. If the value is std::nullopt, the result is ambiguous.
    /// Otherwise, cond is considered either true or false and thus always or never true.
    std::optional<bool> _reachabilityAssignment;

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

    /// Add a condition to the expression. Creates an or statement between the current condition and
    /// the new argument.
    void addCondition(const IR::Expression *cond);

    /// @returns the current reachability value.
    [[nodiscard]] std::optional<bool> getReachability() const;

    /// @returns the reachability expression.
    [[nodiscard]] const IR::Expression *getCondition() const;
};

/// Convenience type definition for a mapping of nodes to a set of reachability expressions.
using ReachabilityMap = std::map<const IR::Node *, ReachabilityExpression *, SourceIdCmp>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_EXPRESSION_H_ */
