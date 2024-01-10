#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"

#include <fcntl.h>

#include <cerrno>
#include <cstdlib>
#include <optional>

#include "ir/irutils.h"
#include "p4/v1/p4runtime.pb.h"

namespace P4Tools::Flay {

big_int ProtobufDeserializer::protoValueToBigInt(const std::string &valueString) {
    big_int value;
    boost::multiprecision::import_bits(value, valueString.begin(), valueString.end());
    return value;
}

std::optional<TableKeySet> ProtobufDeserializer::produceTableMatch(const p4::v1::FieldMatch &field,
                                                                   cstring tableName,
                                                                   cstring keyFieldName,
                                                                   const IR::Expression &keyExpr,
                                                                   SymbolSet &symbolSet) {
    TableKeySet tableKeySet;
    const auto *keySymbol = ControlPlaneState::getTableKey(tableName, keyFieldName, keyExpr.type);
    symbolSet.emplace(*keySymbol);
    if (field.has_exact()) {
        auto value = protoValueToBigInt(field.exact().value());
        tableKeySet.emplace(*keySymbol, *IR::getConstant(keyExpr.type, value));
    } else if (field.has_lpm()) {
        const auto *lpmPrefixSymbol =
            ControlPlaneState::getTableMatchLpmPrefix(tableName, keyFieldName, keyExpr.type);
        symbolSet.emplace(*lpmPrefixSymbol);
        auto value = protoValueToBigInt(field.lpm().value());
        int prefix = field.lpm().prefix_len();
        tableKeySet.emplace(*keySymbol, *IR::getConstant(keyExpr.type, value));
        tableKeySet.emplace(*lpmPrefixSymbol, *IR::getConstant(keyExpr.type, prefix));
    } else if (field.has_ternary()) {
        const auto *maskSymbol =
            ControlPlaneState::getTableTernaryMask(tableName, keyFieldName, keyExpr.type);
        symbolSet.emplace(*maskSymbol);
        auto value = protoValueToBigInt(field.ternary().value());
        auto mask = protoValueToBigInt(field.ternary().mask());
        tableKeySet.emplace(*keySymbol, *IR::getConstant(keyExpr.type, value));
        tableKeySet.emplace(*maskSymbol, *IR::getConstant(keyExpr.type, mask));
    } else {
        ::error("Unsupported table match type %1%.", field.DebugString().c_str());
        return std::nullopt;
    }
    return tableKeySet;
}

const IR::Expression *ProtobufDeserializer::convertTableAction(const p4::v1::Action &tblAction,
                                                               cstring tableName,
                                                               const IR::P4Action &p4Action,
                                                               SymbolSet &symbolSet) {
    const auto *tableActionID = ControlPlaneState::getTableActionChoice(tableName);
    symbolSet.emplace(*tableActionID);
    auto actionName = p4Action.controlPlaneName();
    const auto *actionAssignment = new IR::StringLiteral(actionName);
    const IR::Expression *actionExpr = new IR::Equ(tableActionID, actionAssignment);
    for (const auto &paramConfig : tblAction.params()) {
        const auto *param = p4Action.parameters->getParameter(paramConfig.param_id() - 1);
        auto paramName = param->controlPlaneName();
        const auto *actionArg =
            ControlPlaneState::getTableActionArg(tableName, actionName, paramName, param->type);
        symbolSet.emplace(*actionArg);
        const auto *actionVal =
            IR::getConstant(param->type, protoValueToBigInt(paramConfig.value()));
        actionExpr = new IR::LAnd(actionExpr, new IR::Equ(actionArg, actionVal));
    }
    return actionExpr;
}

std::optional<TableMatchEntry *> ProtobufDeserializer::produceTableEntry(
    cstring tableName, P4::ControlPlaneAPI::p4rt_id_t tblId,
    const P4RuntimeIdtoIrNodeMap &irToIdMap, const p4::v1::TableEntry &tableEntry,
    SymbolSet &symbolSet) {
    RETURN_IF_FALSE_WITH_MESSAGE(
        tableEntry.action().has_action(), std::nullopt,
        ::error("Table entry %1% has no action.", tableEntry.DebugString()));

    auto tblAction = tableEntry.action().action();
    ASSIGN_OR_RETURN_WITH_MESSAGE(
        const auto *actionResult, safeAt(irToIdMap, tblAction.action_id()), std::nullopt,
        ::error("ID %1% not found in the irToIdMap map.", tblAction.action_id()));
    ASSIGN_OR_RETURN_WITH_MESSAGE(auto &p4Action, actionResult->to<IR::P4Action>(), std::nullopt,
                                  ::error("%1% is not a IR::P4Action.", actionResult));

    const auto *actionExpr = convertTableAction(tblAction, tableName, p4Action, symbolSet);

    TableKeySet tableKeySet;
    for (const auto &field : tableEntry.match()) {
        auto fieldId = P4::ControlPlaneAPI::szudzikPairing(tblId, field.field_id());
        ASSIGN_OR_RETURN_WITH_MESSAGE(const auto *result, safeAt(irToIdMap, fieldId), std::nullopt,
                                      ::error("ID %1% not found in the irToIdMap map.", fieldId));

        ASSIGN_OR_RETURN_WITH_MESSAGE(auto &keyField, result->to<IR::KeyElement>(), std::nullopt,
                                      ::error("%1% is not a IR::KeyElement.", result));
        ASSIGN_OR_RETURN_WITH_MESSAGE(auto &nameAnnot, keyField.getAnnotation("name"), std::nullopt,
                                      ::error("Non-constant table key without an annotation"));

        ASSIGN_OR_RETURN(auto matchSet,
                         produceTableMatch(field, tableName, nameAnnot.getName(),
                                           *keyField.expression, symbolSet),
                         std::nullopt);

        tableKeySet.insert(matchSet.begin(), matchSet.end());
    }

    return new TableMatchEntry(actionExpr, tableEntry.priority(), tableKeySet);
}

int ProtobufDeserializer::updateTableEntry(const P4RuntimeIdtoIrNodeMap &irToIdMap,
                                           const p4::v1::TableEntry &tableEntry,
                                           ControlPlaneConstraints &controlPlaneConstraints,
                                           const ::p4::v1::Update_Type &updateType,
                                           SymbolSet &symbolSet) {
    auto tblId = tableEntry.table_id();
    ASSIGN_OR_RETURN_WITH_MESSAGE(const auto *result, safeAt(irToIdMap, tblId), EXIT_FAILURE,
                                  ::error("ID %1% not found in the irToIdMap map.", tblId));
    ASSIGN_OR_RETURN_WITH_MESSAGE(auto &tbl, result->to<IR::P4Table>(), EXIT_FAILURE,
                                  ::error("Table %1% is not a IR::P4Table.", result));
    cstring tableName = tbl.controlPlaneName();

    auto it = controlPlaneConstraints.find(tableName);
    RETURN_IF_FALSE_WITH_MESSAGE(
        it != controlPlaneConstraints.end(), EXIT_FAILURE,
        ::error("Configuration for table %1% not found in the control plane constraints. It should "
                "have already been initialized at this point.",
                tableName));

    ASSIGN_OR_RETURN_WITH_MESSAGE(
        auto &tableResult, it->second.get().to<TableConfiguration>(), EXIT_FAILURE,
        ::error("Configuration result is not a TableConfiguration.", tableName));

    ASSIGN_OR_RETURN(auto *tableMatchEntry,
                     produceTableEntry(tableName, tblId, irToIdMap, tableEntry, symbolSet),
                     EXIT_FAILURE);

    if (updateType == p4::v1::Update::MODIFY) {
        tableResult.addTableEntry(*tableMatchEntry, true);
    } else if (updateType == p4::v1::Update::INSERT) {
        RETURN_IF_FALSE_WITH_MESSAGE(
            tableResult.addTableEntry(*tableMatchEntry, false) == EXIT_SUCCESS, EXIT_FAILURE,
            ::error("Table entry \"%1%\" already exists.", tableEntry.ShortDebugString()));
    } else if (updateType == p4::v1::Update::DELETE) {
        RETURN_IF_FALSE_WITH_MESSAGE(tableResult.deleteTableEntry(*tableMatchEntry) != 0,
                                     EXIT_FAILURE,
                                     ::error("Table entry %1% not found and can not be deleted.",
                                             tableEntry.ShortDebugString()));
    } else {
        ::error("Unsupported update type %1%.", updateType);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
    const p4::v1::Entity &entity, const P4RuntimeIdtoIrNodeMap &irToIdMap,
    ControlPlaneConstraints &controlPlaneConstraints, const ::p4::v1::Update_Type &updateType,
    SymbolSet &symbolSet) {
    if (entity.has_table_entry()) {
        RETURN_IF_FALSE(updateTableEntry(irToIdMap, entity.table_entry(), controlPlaneConstraints,
                                         updateType, symbolSet) == EXIT_SUCCESS,
                        EXIT_FAILURE)
    } else {
        ::error("Unsupported control plane entry %1%.", entity.DebugString().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int ProtobufDeserializer::updateControlPlaneConstraints(
    const flaytests::Config &protoControlPlaneConfig, const P4RuntimeIdtoIrNodeMap &irToIdMap,
    ControlPlaneConstraints &controlPlaneConstraints, SymbolSet &symbolSet) {
    for (const auto &entity : protoControlPlaneConfig.entities()) {
        if (updateControlPlaneConstraintsWithEntityMessage(
                entity, irToIdMap, controlPlaneConstraints, p4::v1::Update::MODIFY, symbolSet) !=
            EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

}  // namespace P4Tools::Flay
