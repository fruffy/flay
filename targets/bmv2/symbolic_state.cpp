#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/control_plane/p4runtime_api.h"
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

int ProtobufBmv2ControlPlaneState::initializeDefaultState(const p4::config::v1::P4Info &p4Info) {
    for (const auto &table : p4Info.tables()) {
        auto tableName = table.preamble().name();

        auto defaultActionId = table.const_default_action_id();
        std::optional<p4::config::v1::Action> defaultActionOpt;
        // If the default action id is 0, look for the default action in the P4 info file.
        if (defaultActionId == 0) {
            defaultActionOpt = P4::ControlPlaneAPI::findP4RuntimeAction(p4Info, "NoAction");
        } else {
            defaultActionOpt = P4::ControlPlaneAPI::findP4RuntimeAction(p4Info, defaultActionId);
        }
        if (!defaultActionOpt.has_value()) {
            ::error("Failed to find default action id %1% in the P4Info object", defaultActionId);
            return EXIT_FAILURE;
        }
        auto &defaultAction = defaultActionOpt.value();

        auto defaultEntry =
            TableMatchEntry(new IR::Equ(ControlPlaneState::getTableActionChoice(tableName),
                                        new IR::StringLiteral(defaultAction.preamble().name())),
                            0, {});
        defaultConstraints.insert(
            {tableName, *new TableConfiguration(tableName, defaultEntry, {})});
    }
    defaultConstraints.emplace("clone_session", *new CloneSession(std::nullopt));

    return EXIT_SUCCESS;
}

}  // namespace P4Tools::Flay::V1Model
