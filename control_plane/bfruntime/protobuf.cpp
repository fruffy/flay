#include "backends/p4tools/modules/flay/control_plane/bfruntime/protobuf.h"

#include <fcntl.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf_utils.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "control-plane/p4infoApi.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::BfRuntime {

std::optional<TableKeySet> produceTableMatch(const bfrt_proto::KeyField &field, cstring tableName,
                                             const p4::config::v1::MatchField &matchField,
                                             SymbolSet &symbolSet) {
    TableKeySet tableKeySet;
    const auto *keyType = IR::Type_Bits::get(matchField.bitwidth());
    const auto *keySymbol = ControlPlaneState::getTableKey(tableName, matchField.name(), keyType);
    symbolSet.emplace(*keySymbol);
    switch (field.match_type_case()) {
        case bfrt_proto::KeyField::kExact: {
            auto value = Protobuf::stringToBigInt(field.exact().value());
            tableKeySet.emplace(*keySymbol, *IR::Constant::get(keyType, value));
            return tableKeySet;
        }
        case bfrt_proto::KeyField::kLpm: {
            const auto *lpmPrefixSymbol =
                ControlPlaneState::getTableMatchLpmPrefix(tableName, matchField.name(), keyType);
            symbolSet.emplace(*lpmPrefixSymbol);
            auto value = Protobuf::stringToBigInt(field.lpm().value());
            int prefix = field.lpm().prefix_len();
            tableKeySet.emplace(*keySymbol, *IR::Constant::get(keyType, value));
            tableKeySet.emplace(*lpmPrefixSymbol, *IR::Constant::get(keyType, prefix));
            return tableKeySet;
        }
        case bfrt_proto::KeyField::kTernary: {
            const auto *maskSymbol =
                ControlPlaneState::getTableTernaryMask(tableName, matchField.name(), keyType);
            symbolSet.emplace(*maskSymbol);
            auto value = Protobuf::stringToBigInt(field.ternary().value());
            auto mask = Protobuf::stringToBigInt(field.ternary().mask());
            tableKeySet.emplace(*keySymbol, *IR::Constant::get(keyType, value));
            tableKeySet.emplace(*maskSymbol, *IR::Constant::get(keyType, mask));
            return tableKeySet;
        }
        default:
            ::error("Unsupported table match type %1%.", field.DebugString().c_str());
    }
    return std::nullopt;
}

std::optional<TableKeySet> produceTableMatchForMissingField(
    cstring tableName, const p4::config::v1::MatchField &matchField, SymbolSet &symbolSet) {
    TableKeySet tableKeySet;
    const auto *keyType = IR::Type_Bits::get(matchField.bitwidth());
    const auto *keySymbol = ControlPlaneState::getTableKey(tableName, matchField.name(), keyType);
    symbolSet.emplace(*keySymbol);
    switch (matchField.match_type()) {
        /// We can convert missing ternary and optional fields to 0.
        case p4::config::v1::MatchField::TERNARY:
        case p4::config::v1::MatchField::OPTIONAL: {
            const auto *keySymbol =
                ControlPlaneState::getTableKey(tableName, matchField.name(), keyType);
            const auto *maskSymbol =
                ControlPlaneState::getTableTernaryMask(tableName, matchField.name(), keyType);
            symbolSet.emplace(*keySymbol);
            symbolSet.emplace(*maskSymbol);
            tableKeySet.emplace(*keySymbol, *IR::Constant::get(keyType, 0));
            tableKeySet.emplace(*maskSymbol, *IR::Constant::get(keyType, 0));
            return tableKeySet;
        }
        default:
            ::error("Unsupported match type %1%.", matchField.DebugString());
    }
    return std::nullopt;
}

std::optional<const IR::Expression *> convertTableAction(const bfrt_proto::TableData &tblAction,
                                                         cstring tableName,
                                                         const p4::config::v1::Action &p4Action,
                                                         SymbolSet &symbolSet,
                                                         bool isDefaultAction) {
    const IR::SymbolicVariable *tableActionID =
        isDefaultAction ? ControlPlaneState::getDefaultActionVariable(tableName)
                        : ControlPlaneState::getTableActionChoice(tableName);
    symbolSet.emplace(*tableActionID);
    auto actionName = p4Action.preamble().name();
    const auto *actionAssignment = IR::StringLiteral::get(actionName);
    const IR::Expression *actionExpr = new IR::Equ(tableActionID, actionAssignment);
    if (tblAction.fields().size() != p4Action.params().size()) {
        return actionExpr;
    }
    RETURN_IF_FALSE_WITH_MESSAGE(
        tblAction.fields().size() == p4Action.params().size(), std::nullopt,
        ::error("Action configuration \"%1%\" and target action \"%2%\" "
                "have parameters of different number.",
                p4Action.ShortDebugString(), tblAction.ShortDebugString()));
    for (int idx = 0; idx < tblAction.fields().size(); ++idx) {
        const auto &paramConfig = tblAction.fields().at(idx);
        const auto &param = p4Action.params().at(idx);
        const auto *paramType = IR::Type_Bits::get(param.bitwidth());
        auto paramName = param.name();
        const auto *actionArg =
            ControlPlaneState::getTableActionArgument(tableName, actionName, paramName, paramType);
        symbolSet.emplace(*actionArg);
        RETURN_IF_FALSE_WITH_MESSAGE(paramConfig.has_stream(), std::nullopt,
                                     ::error("Parameter %1% of action %2% is not a stream value.",
                                             paramConfig.DebugString(), actionName));
        const auto *actionVal =
            IR::Constant::get(paramType, Protobuf::stringToBigInt(paramConfig.stream()));
        actionExpr = new IR::LAnd(actionExpr, new IR::Equ(actionArg, actionVal));
    }
    return actionExpr;
}

std::optional<TableMatchEntry *> produceTableEntry(cstring tableName,
                                                   P4::ControlPlaneAPI::p4rt_id_t tblId,
                                                   const p4::config::v1::P4Info &p4Info,
                                                   const bfrt_proto::TableEntry &tableEntry,
                                                   SymbolSet &symbolSet) {
    RETURN_IF_FALSE_WITH_MESSAGE(
        tableEntry.has_data(), std::nullopt,
        ::error("Table entry %1% has no action.", tableEntry.DebugString()));

    const auto &tableAction = tableEntry.data();
    auto actionId = tableAction.action_id();
    ASSIGN_OR_RETURN_WITH_MESSAGE(
        auto &p4Action, P4::ControlPlaneAPI::findP4RuntimeAction(p4Info, actionId), std::nullopt,
        ::error("Action ID %1% not found in the P4Info.", actionId));
    ASSIGN_OR_RETURN(const auto *actionExpr,
                     convertTableAction(tableAction, tableName, p4Action, symbolSet, false),
                     std::nullopt);
    ASSIGN_OR_RETURN_WITH_MESSAGE(
        auto &p4InfoTable, P4::ControlPlaneAPI::findP4RuntimeTable(p4Info, tblId), std::nullopt,
        ::error("Table ID %1% not found in the P4Info.", actionId));

    RETURN_IF_FALSE_WITH_MESSAGE(
        tableEntry.key().fields_size() <= p4InfoTable.match_fields().size(), std::nullopt,
        ::error("Table entry %1% has %2% matches, but P4Info has %3%.", tableEntry.DebugString(),
                tableEntry.key().fields_size(), p4InfoTable.match_fields().size()));
    // Use this map to look up which match fields are present in the control plane entry.
    // TODO: Cache this somehow?
    auto matchMap = std::map<uint32_t, bfrt_proto::KeyField>();
    for (const auto &matchField : tableEntry.key().fields()) {
        matchMap.emplace(matchField.field_id(), matchField);
    }

    TableKeySet tableKeySet;
    for (const auto &p4InfoMatchField : p4InfoTable.match_fields()) {
        auto matchFieldIt = matchMap.find(p4InfoMatchField.id());
        std::optional<TableKeySet> matchSetOpt;
        // If we are missing a match entry, create the dummy entry for supported fields.
        if (matchFieldIt == matchMap.end()) {
            matchSetOpt = produceTableMatchForMissingField(tableName, p4InfoMatchField, symbolSet);
        } else {
            matchSetOpt =
                produceTableMatch(matchFieldIt->second, tableName, p4InfoMatchField, symbolSet);
        }
        ASSIGN_OR_RETURN(auto matchSet, matchSetOpt, std::nullopt);
        tableKeySet.insert(matchSet.begin(), matchSet.end());
    }

    return new TableMatchEntry(actionExpr, 0, tableKeySet);
}

int updateTableEntry(const p4::config::v1::P4Info &p4Info, const bfrt_proto::TableEntry &tableEntry,
                     ControlPlaneConstraints &controlPlaneConstraints,
                     const ::bfrt_proto::Update_Type &updateType, SymbolSet &symbolSet) {
    auto tblId = tableEntry.table_id();
    ASSIGN_OR_RETURN_WITH_MESSAGE(
        auto &p4Table, P4::ControlPlaneAPI::findP4RuntimeTable(p4Info, tblId), EXIT_FAILURE,
        ::error("Table ID %1% not found in the P4Info.", tblId));
    cstring tableName = p4Table.preamble().name();

    auto it = controlPlaneConstraints.find(tableName);
    RETURN_IF_FALSE_WITH_MESSAGE(
        it != controlPlaneConstraints.end(), EXIT_FAILURE,
        ::error("Configuration for table %1% not found in the control plane constraints. It should "
                "have already been initialized at this point.",
                tableName));

    ASSIGN_OR_RETURN_WITH_MESSAGE(
        auto &tableResult, it->second.get().to<TableConfiguration>(), EXIT_FAILURE,
        ::error("Configuration result is not a TableConfiguration.", tableName));

    if (tableEntry.is_default_entry()) {
        const auto &defaultAction = tableEntry.data();
        ASSIGN_OR_RETURN_WITH_MESSAGE(
            auto &p4Action,
            P4::ControlPlaneAPI::findP4RuntimeAction(p4Info, defaultAction.action_id()),
            EXIT_FAILURE,
            ::error("Action ID %1% not found in the P4Info.", defaultAction.action_id()));
        ASSIGN_OR_RETURN(auto defaultActionExpr,
                         convertTableAction(defaultAction, tableName, p4Action, symbolSet, true),
                         EXIT_FAILURE);
        tableResult.setDefaultTableAction(TableDefaultAction(defaultActionExpr));
    }

    RETURN_IF_FALSE_WITH_MESSAGE(
        !p4Table.is_const_table(), EXIT_FAILURE,
        ::error("Trying to insert an entry into table '%1%', which is a const table.", tableName));

    // Consider a delete message without an action a wild card delete.
    if (updateType == bfrt_proto::Update::DELETE && !tableEntry.has_data()) {
        tableResult.clearTableEntries();
        return EXIT_SUCCESS;
    }

    ASSIGN_OR_RETURN(auto *tableMatchEntry,
                     produceTableEntry(tableName, tblId, p4Info, tableEntry, symbolSet),
                     EXIT_FAILURE);

    if (updateType == bfrt_proto::Update::MODIFY) {
        tableResult.addTableEntry(*tableMatchEntry, true);
    } else if (updateType == bfrt_proto::Update::INSERT) {
        RETURN_IF_FALSE_WITH_MESSAGE(
            tableResult.addTableEntry(*tableMatchEntry, false) == EXIT_SUCCESS, EXIT_FAILURE,
            ::error("Table entry \"%1%\" already exists.", tableEntry.ShortDebugString()));
    } else if (updateType == bfrt_proto::Update::DELETE) {
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

int updateControlPlaneConstraintsWithEntityMessage(const bfrt_proto::Entity &entity,
                                                   const p4::config::v1::P4Info &p4Info,
                                                   ControlPlaneConstraints &controlPlaneConstraints,
                                                   const ::bfrt_proto::Update_Type &updateType,
                                                   SymbolSet &symbolSet) {
    if (entity.has_table_entry()) {
        RETURN_IF_FALSE(updateTableEntry(p4Info, entity.table_entry(), controlPlaneConstraints,
                                         updateType, symbolSet) == EXIT_SUCCESS,
                        EXIT_FAILURE)
    } else {
        ::error("Unsupported control plane entry %1%.", entity.DebugString().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int updateControlPlaneConstraints(const bfruntime::flaytests::Config &protoControlPlaneConfig,
                                  const p4::config::v1::P4Info &p4Info,
                                  ControlPlaneConstraints &controlPlaneConstraints,
                                  SymbolSet &symbolSet) {
    for (const auto &entity : protoControlPlaneConfig.entities()) {
        if (updateControlPlaneConstraintsWithEntityMessage(entity, p4Info, controlPlaneConstraints,
                                                           bfrt_proto::Update::MODIFY,
                                                           symbolSet) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

}  // namespace P4Tools::Flay::BfRuntime
