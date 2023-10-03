#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <stdio.h>

#include <optional>

#include "lib/error.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const ExecutionState &executionState) : executionState(executionState) {}

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

    auto solverResult = solver.checkSat({condition});
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

void ElimDeadCode::end_apply() {
    if (deletedCode) {
        printf("Dead code found.\n");
    } else {
        printf("No dead code found.\n");
    }
}

}  // namespace P4Tools::Flay
