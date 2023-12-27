#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/targets/bmv2/control_plane_objects.h"

namespace P4Tools::Flay::V1Model {

const IR::SymbolicVariable *Bmv2ControlPlaneState::getCloneActive() {
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::getSessionId(const IR::Type *type) {
    return ToolsVariables::getSymbolicVariable(type, "clone_session_id");
}

int Bmv2ControlPlaneState::allocateControlPlaneTable(const IR::P4Table &table) {
    auto tableName = table.controlPlaneName();
    const auto *defaultAction = table.getDefaultAction();
    auto actionName = defaultAction->checkedTo<IR::MethodCallExpression>()
                          ->method->checkedTo<IR::PathExpression>()
                          ->path->name;
    auto defaultEntry =
        TableMatchEntry(new IR::Equ(ControlPlaneState::getTableActionChoice(tableName),
                                    new IR::StringLiteral(actionName)),
                        0, {});
    defaultConstraints.insert({tableName, *new TableConfiguration(tableName, defaultEntry, {})});
    return EXIT_SUCCESS;
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::allocateCloneSession() {
    auto *cloneSession = new CloneSession(std::nullopt);
    defaultConstraints.emplace("clone_session", *cloneSession);
    const auto *var = getCloneActive();
    return var;
}

}  // namespace P4Tools::Flay::V1Model
