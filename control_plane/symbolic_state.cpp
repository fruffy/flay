#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"

#include "backends/p4tools/common/lib/variables.h"

namespace P4Tools::Flay {
ControlPlaneConstraints ControlPlaneState::getDefaultConstraints() const {
    return defaultConstraints;
}

const IR::SymbolicVariable *ControlPlaneState::getTableActive(cstring tableName) {
    cstring label = tableName + "_configured";
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableKey(cstring tableName, cstring fieldName,
                                                           const IR::Type *type) {
    cstring label = tableName + "_key_" + fieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableTernaryMask(cstring tableName,
                                                                   cstring fieldName,
                                                                   const IR::Type *type) {
    cstring label = tableName + "_mask_" + fieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableMatchLpmPrefix(cstring tableName,
                                                                      cstring fieldName,
                                                                      const IR::Type *type) {
    cstring label = tableName + "_lpm_prefix_" + fieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableActionArg(cstring tableName,
                                                                 cstring actionName,
                                                                 cstring parameterName,
                                                                 const IR::Type *type) {
    cstring label = tableName + "_" + actionName + "_param_" + parameterName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableActionChoice(cstring tableName) {
    cstring label = tableName + "_action";
    return ToolsVariables::getSymbolicVariable(IR::Type_String::get(), label);
}

}  // namespace P4Tools::Flay
