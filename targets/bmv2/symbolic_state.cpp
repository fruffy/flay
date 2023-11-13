#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

#include "backends/p4tools/common/lib/variables.h"

namespace P4Tools::Flay::V1Model {

const IR::SymbolicVariable *Bmv2ControlPlaneState::getCloneActive() {
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::getSessionId(const IR::Type *type) {
    return ToolsVariables::getSymbolicVariable(type, "clone_session_id");
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::allocateControlPlaneTable(cstring tableName) {
    auto var = getTableActive(tableName);
    defaultConstraints.emplace(*var, IR::getBoolLiteral(false));
    return var;
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::allocateCloneSession() {
    auto var = getCloneActive();
    defaultConstraints.emplace(*var, IR::getBoolLiteral(false));
    return var;
}

}  // namespace P4Tools::Flay::V1Model
