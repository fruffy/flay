#include "backends/p4tools/modules/flay/core/reachability_expression.h"

namespace P4Tools::Flay {

const IR::Expression *ReachabilityExpression::getCondition() const { return _cond; }

void ReachabilityExpression::setCondition(const IR::Expression *cond) { _cond = cond; }

std::optional<bool> ReachabilityExpression::getReachability() const {
    return _reachabilityAssignment;
}

void ReachabilityExpression::setReachability(std::optional<bool> reachability) {
    _reachabilityAssignment = reachability;
}

ReachabilityExpression::ReachabilityExpression(const IR::Expression *cond,
                                               std::optional<bool> reachabilityAssignment)
    : _cond(cond), _reachabilityAssignment(reachabilityAssignment) {}

ReachabilityExpression::ReachabilityExpression(const IR::Expression *cond)
    : _cond(cond), _reachabilityAssignment(std::nullopt) {}

}  // namespace P4Tools::Flay
