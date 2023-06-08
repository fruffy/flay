#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const ExecutionState &executionState) : executionState(executionState) {}

const IR::Node *ElimDeadCode::preorder(IR::IfStatement *stmt) {
    const auto *condition = executionState.get().getReachabilityCondition(stmt);

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
