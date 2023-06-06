#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

namespace P4Tools::Flay {

const IR::Node *ElimDeadCode::preorder(IR::IfStatement *stmt) {
    const auto *condition = executionState.get().getReachabilityCondition(stmt);

    auto solverResult = solver.checkSat({condition});
    if (solverResult == std::nullopt) {
        return stmt;
    }
    /// There is no way to satisfy the condition. This means we can not ever execute the true
    /// branch.
    if (!solverResult.value()) {
        if (stmt->ifFalse != nullptr) {
            return stmt->ifFalse;
        }
        return new IR::EmptyStatement();
    }
    return stmt;
}

}  // namespace P4Tools::Flay
