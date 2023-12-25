#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <optional>

#include "ir/ir-generated.h"
#include "lib/error.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const ReachabilityMap &reachabilityMap)
    : reachabilityMap(reachabilityMap) {}

const IR::Node *ElimDeadCode::postorder(IR::IfStatement *stmt) {
    auto conditionOpt = reachabilityMap.get().getReachabilityExpression(stmt);
    if (!conditionOpt.has_value()) {
        ::error(
            "Unable to find node %1% in the reachability map of this execution state. There might "
            "be "
            "issues with the source information.",
            stmt);
        return stmt;
    }
    auto condition = conditionOpt.value();
    auto reachability = condition.getReachability();

    if (reachability) {
        ::warning("%1% condition can be deleted.", stmt->condition);
        if (reachability.value()) {
            if (stmt->ifFalse != nullptr) {
                ::warning("%1% false branch can be deleted.", stmt->ifFalse);
            }
            return stmt->ifTrue;
        }
        ::warning("%1% true branch can be deleted.", stmt->ifTrue);
        if (stmt->ifFalse != nullptr) {
            return stmt->ifFalse;
        }
        return new IR::EmptyStatement();
    }
    return stmt;
}

}  // namespace P4Tools::Flay
