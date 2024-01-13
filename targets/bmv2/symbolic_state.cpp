#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/control_plane/p4runtime_api.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
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

int ProtobufBmv2ControlPlaneState::initializeDefaultState(const p4::config::v1::P4Info &p4Info,
                                                          const P4RuntimeIdtoIrNodeMap &irToIdMap,
                                                          const P4::ReferenceMap &refMap) {
    for (const auto &table : p4Info.tables()) {
        // Do not add any default constraints for immutable tables.
        if (table.is_const_table()) {
            continue;
        }
        auto tableName = table.preamble().name();

        auto defaultActionId = table.const_default_action_id();
        std::optional<p4::config::v1::Action> defaultActionOpt;
        // If the default action id is 0, we need to look up the table object and get the default
        // action.
        // TODO: Get rid of this convoluted list of lookups and checks. This only exists because of
        // a limitation in the P4Info file.
        if (defaultActionId == 0) {
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto *result, safeAt(irToIdMap, table.preamble().id()), EXIT_FAILURE,
                ::error("ID %1% not found in the irToIdMap map.", table.preamble().id()));
            ASSIGN_OR_RETURN_WITH_MESSAGE(auto &tbl, result->to<IR::P4Table>(), EXIT_FAILURE,
                                          ::error("Table %1% is not a IR::P4Table.", result));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                auto &defaultActionRef, tbl.getDefaultAction(), EXIT_FAILURE,
                ::error("Table %1% does not have a default action.", tbl));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &tableAction, defaultActionRef.to<IR::MethodCallExpression>(),
                EXIT_FAILURE,
                ::error("Action reference %1% is not a method call expression.", defaultActionRef));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &actionPath, tableAction.method->to<IR::PathExpression>(), EXIT_FAILURE,
                ::error("Action reference %1% is not a path expression.", tableAction.method));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &actionDecl, refMap.getDeclaration(actionPath.path), EXIT_FAILURE,
                ::error("Unable to find action declaration %1%.", actionPath.path));
            defaultActionOpt =
                P4::ControlPlaneAPI::findP4RuntimeAction(p4Info, actionDecl.controlPlaneName());
        } else {
            defaultActionOpt = P4::ControlPlaneAPI::findP4RuntimeAction(p4Info, defaultActionId);
        }
        ASSIGN_OR_RETURN(auto &defaultAction, defaultActionOpt, EXIT_FAILURE);
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
