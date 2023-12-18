#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "lib/error.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const ExecutionState &executionState, AbstractSolver &solver)
    : executionState(executionState), solver(solver) {}

const IR::Node *ElimDeadCode::postorder(IR::IfStatement *stmt) {
    const auto *condition = executionState.get().getReachabilityCondition(stmt, false);
    if (condition == nullptr) {
        ::warning(
            "Unable to find node %1% in the reachability map of this execution state. There might "
            "be "
            "issues with the source information.",
            stmt);
        return stmt;
    }

    /// Merge the execution condition with the overall constraints.
    std::vector<const Constraint *> mergedConstraints(controlPlaneConstraintExprs);
    mergedConstraints.push_back(condition);
    auto solverResult = solver.get().checkSat(mergedConstraints);
    if (solverResult == std::nullopt) {
        return stmt;
    }
    /// There is no way to satisfy the condition. This means we can not ever
    /// execute the true branch.
    if (!solverResult.value()) {
        deletedCode = true;
        ::warning("%1% can be deleted.", stmt->ifTrue);
        if (stmt->ifFalse != nullptr) {
            return stmt->ifFalse;
        }
        return new IR::EmptyStatement();
    }
    return stmt;
}

ElimDeadCode::profile_t ElimDeadCode::init_apply(const IR::Node *root) {
    for (auto &[entityName, controlPlaneConstraint] : controlPlaneConstraints) {
        controlPlaneConstraintExprs.push_back(
            controlPlaneConstraint.get().computeControlPlaneConstraint());
    }
    return Transform::init_apply(root);
}

void ElimDeadCode::end_apply() {
    if (deletedCode) {
        printInfo("Dead code found.\n");
    } else {
        printInfo("No dead code found.\n");
    }
}

void ElimDeadCode::addControlPlaneConstraints(
    const ControlPlaneConstraints &newControlPlaneConstraints) {
    controlPlaneConstraints.insert(newControlPlaneConstraints.begin(),
                                   newControlPlaneConstraints.end());
}

ControlPlaneConstraints &ElimDeadCode::getWriteableControlPlaneConstraints() {
    return controlPlaneConstraints;
}

void ElimDeadCode::removeControlPlaneConstraints(
    const ControlPlaneConstraints &newControlPlaneConstraints) {
    for (const auto &constraint : newControlPlaneConstraints) {
        controlPlaneConstraints.erase(constraint.first);
    }
}

}  // namespace P4Tools::Flay
