#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_

#include <map>
#include <optional>

#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// Utility function to compare IR nodes in a set. We use their source info.
struct SourceIdCmp {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const;
};

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

    void setReachability(std::optional<bool> reachability);
    void setCondition(const IR::Expression *cond);

    [[nodiscard]] std::optional<bool> getReachability() const;

    [[nodiscard]] const IR::Expression *getCondition() const;
};

class ReachabilityMap : private std::map<const IR::Node *, ReachabilityExpression, SourceIdCmp> {
 public:
    [[nodiscard]] std::optional<ReachabilityExpression> getReachabilityExpression(
        const IR::Node *node) const;

    bool updateReachabilityAssignment(const IR::Node *node, std::optional<bool> reachability);

    std::optional<bool> recomputeReachability(
        AbstractSolver &solver, const ControlPlaneConstraints &initialControlPlaneConstraints);

    std::optional<bool> computeNodeReachability(
        const IR::Node *node, AbstractSolver &solver,
        const std::vector<const Constraint *> &controlPlaneConstraintExprs);

    bool initializeReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    void mergeReachabilityMapping(const ReachabilityMap &otherMap);

    void substitutePlaceholders(Transform &substitute);
};

}  // namespace P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_REACHABILITY_H_ */
