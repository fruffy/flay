#include "backends/p4tools/modules/flay/core/reachability.h"

#include "lib/error.h"

namespace P4Tools::Flay {

const IR::Expression *ReachabilityExpression::getCondition() const { return cond; }

void ReachabilityExpression::setCondition(const IR::Expression *cond) { this->cond = cond; }

std::optional<bool> ReachabilityExpression::getReachability() const {
    return reachabilityAssignment;
}

void ReachabilityExpression::setReachability(std::optional<bool> reachability) {
    reachabilityAssignment = reachability;
}

ReachabilityExpression::ReachabilityExpression(const IR::Expression *cond,
                                               std::optional<bool> reachabilityAssignment)
    : cond(cond), reachabilityAssignment(reachabilityAssignment) {}
ReachabilityExpression::ReachabilityExpression(const IR::Expression *cond)
    : cond(cond), reachabilityAssignment(std::nullopt) {}

std::optional<ReachabilityExpression> ReachabilityMap::getReachabilityExpression(
    const IR::Node *node) const {
    auto it = find(node);
    if (it != end()) {
        return it->second;
    }

    return std::nullopt;
}

bool ReachabilityMap::updateReachabilityAssignment(const IR::Node *node,
                                                   std::optional<bool> reachability) {
    auto it = find(node);
    if (it != end()) {
        it->second.setReachability(reachability);
        return true;
    }
    return false;
}

std::optional<bool> ReachabilityMap::recomputeReachability(
    AbstractSolver &solver, const ControlPlaneConstraints &controlPlaneConstraints) {
    /// Generate IR equalities from the control plane constraints.
    std::vector<const Constraint *> controlPlaneConstraintExprs;
    for (const auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        controlPlaneConstraintExprs.push_back(
            controlPlaneConstraint.get().computeControlPlaneConstraint());
    }

    bool hasChanged = false;
    for (auto &pair : *this) {
        auto result = computeNodeReachability(pair.first, solver, controlPlaneConstraintExprs);
        if (!result.has_value()) {
            return std::nullopt;
        }
        hasChanged |= result.value();
    }
    return hasChanged;
}

std::optional<bool> ReachabilityMap::computeNodeReachability(
    const IR::Node *node, AbstractSolver &solver,
    const std::vector<const Constraint *> &controlPlaneConstraintExprs) {
    auto it = find(node);
    if (it == end()) {
        ::error("Reachability mapping for node %1% does not exist.", node);
        return std::nullopt;
    }
    const auto *reachabilityCondition = it->second.getCondition();
    auto reachabilityAssignment = it->second.getReachability();

    std::vector<const Constraint *> mergedConstraints(controlPlaneConstraintExprs);
    mergedConstraints.push_back(reachabilityCondition);
    auto solverResult = solver.checkSat(mergedConstraints);
    /// Solver returns unknown, better leave this alone.
    if (solverResult == std::nullopt) {
        ::warning("Solver returned unknown result for %1%.", node);
        return reachabilityAssignment.has_value();
    }

    bool hasChanged = false;
    /// There is no way to satisfy the condition. It is always false.
    if (!solverResult.value()) {
        it->second.setReachability(false);
        hasChanged = !reachabilityAssignment.has_value() || reachabilityAssignment.value();
        return hasChanged;
    }

    std::vector<const Constraint *> mergedConstraints1(controlPlaneConstraintExprs);
    mergedConstraints1.push_back(new IR::LNot(reachabilityCondition));
    auto solverResult1 = solver.checkSat(mergedConstraints1);
    /// Solver returns unknown, better leave this alone.
    if (solverResult1 == std::nullopt) {
        ::warning("Solver returned unknown result for %1%.", node);
        return reachabilityAssignment.has_value();
    }
    /// There is no way to falsify the condition. It is always true.
    if (!solverResult1.value()) {
        it->second.setReachability(true);
        hasChanged = !reachabilityAssignment.has_value() || !reachabilityAssignment.value();
        return hasChanged;
    }

    if (solverResult.value() && solverResult1.value()) {
        it->second.setReachability(std::nullopt);
        hasChanged = reachabilityAssignment.has_value();
    }

    return hasChanged;
}

bool ReachabilityMap::initializeReachabilityMapping(const IR::Node *node,
                                                    const IR::Expression *cond) {
    return emplace(node, cond).second;
}

void ReachabilityMap::mergeReachabilityMapping(const ReachabilityMap &otherMap) {
    for (const auto &rechabilityTuple : otherMap) {
        insert(rechabilityTuple);
    }
}

void ReachabilityMap::substitutePlaceholders(Transform &substitute) {
    for (auto &[node, reachabilityExpression] : *this) {
        reachabilityExpression.setCondition(
            reachabilityExpression.getCondition()->apply(substitute));
    }
}

}  // namespace P4Tools::Flay
