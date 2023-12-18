#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

#include <optional>

#include "backends/p4tools/common/lib/variables.h"

namespace P4Tools::Flay::V1Model {

const IR::SymbolicVariable *Bmv2ControlPlaneState::getCloneActive() {
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::getSessionId(const IR::Type *type) {
    return ToolsVariables::getSymbolicVariable(type, "clone_session_id");
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::allocateControlPlaneTable(
    const IR::P4Table &table) {
    const auto *defaultAction = table.getDefaultAction();
    auto tableName = table.controlPlaneName();
    auto defaultEntry = TableMatchEntry(
        new IR::Equ(ControlPlaneState::getTableActionChoice(tableName),
                    new IR::StringLiteral(defaultAction->checkedTo<IR::MethodCallExpression>()
                                              ->method->checkedTo<IR::PathExpression>()
                                              ->path->name)),
        0);
    auto *tableConfig = new TableConfiguration(tableName, defaultEntry, {});
    defaultConstraints.insert({tableName, *tableConfig});
    const auto *var = getTableActive(tableName);
    return var;
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::allocateCloneSession() {
    auto *cloneSession = new CloneSession(std::nullopt);
    defaultConstraints.emplace("clone_session", *cloneSession);
    const auto *var = getCloneActive();
    return var;
}

}  // namespace P4Tools::Flay::V1Model
