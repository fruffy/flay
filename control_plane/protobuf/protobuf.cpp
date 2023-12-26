#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"

#include <fcntl.h>

#include <google/protobuf/text_format.h>

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

big_int ProtobufDeserializer::protoValueToBigInt(const std::string &valueString) {
    big_int value;
    boost::multiprecision::import_bits(value, valueString.begin(), valueString.end());
    return value;
}

std::optional<flaytests::Config> ProtobufDeserializer::deserializeProtobufConfig(
    const std::filesystem::path &inputFile) {
    flaytests::Config protoControlPlaneConfig;

    // Parse the input file into the Protobuf object.
    int fd = open(inputFile.c_str(), O_RDONLY);
    google::protobuf::io::ZeroCopyInputStream *input =
        new google::protobuf::io::FileInputStream(fd);

    if (google::protobuf::TextFormat::Parse(input, &protoControlPlaneConfig)) {
        printInfo("Parsed configuration: %1%", protoControlPlaneConfig.DebugString());
    } else {
        ::error("Failed to parse configuration: %1%", protoControlPlaneConfig.ShortDebugString());
        return std::nullopt;
    }
    // Close the open file.
    close(fd);
    return protoControlPlaneConfig;
}

std::optional<TableKeySet> ProtobufDeserializer::produceTableMatch(const p4::v1::FieldMatch &field,
                                                                   cstring tableName,
                                                                   cstring keyFieldName,
                                                                   const IR::Expression &keyExpr) {
    TableKeySet tableKeySet;
    const auto *keySymbol = ControlPlaneState::getTableKey(tableName, keyFieldName, keyExpr.type);
    if (field.has_exact()) {
        auto value = protoValueToBigInt(field.exact().value());
        tableKeySet.emplace(*keySymbol, *IR::getConstant(keyExpr.type, value));
    } else if (field.has_lpm()) {
        const auto *lpmPrefixSymbol =
            ControlPlaneState::getTableMatchLpmPrefix(tableName, keyFieldName, keyExpr.type);
        auto value = protoValueToBigInt(field.lpm().value());
        int prefix = field.lpm().prefix_len();
        tableKeySet.emplace(*keySymbol, *IR::getConstant(keyExpr.type, value));
        tableKeySet.emplace(*lpmPrefixSymbol, *IR::getConstant(keyExpr.type, prefix));
    } else if (field.has_ternary()) {
        const auto *maskSymbol =
            ControlPlaneState::getTableTernaryMask(tableName, keyFieldName, keyExpr.type);
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
                                                               const IR::P4Action &p4Action) {
    const auto *tableActionID = ControlPlaneState::getTableActionChoice(tableName);

    auto actionName = p4Action.controlPlaneName();
    const auto *actionAssignment = new IR::StringLiteral(actionName);
    const IR::Expression *actionExpr = new IR::Equ(tableActionID, actionAssignment);
    for (const auto &paramConfig : tblAction.params()) {
        const auto *param = p4Action.parameters->getParameter(paramConfig.param_id() - 1);
        auto paramName = param->controlPlaneName();
        const auto &actionArg =
            ControlPlaneState::getTableActionArg(tableName, actionName, paramName, param->type);
        const auto *actionVal =
            IR::getConstant(param->type, protoValueToBigInt(paramConfig.value()));
        actionExpr = new IR::LAnd(actionExpr, new IR::Equ(actionArg, actionVal));
    }
    return actionExpr;
}

int ProtobufDeserializer::convertTableEntry(const P4RuntimeIdtoIrNodeMap &irToIdMap,
                                            const p4::v1::TableEntry &tableEntry,
                                            ControlPlaneConstraints &controlPlaneConstraints) {
    auto tblId = tableEntry.table_id();
    const auto *result = irToIdMap.at(tblId);
    if (result->to<IR::P4Table>() == nullptr) {
        ::error("Table %1% is not a IR::P4Table.", result);
        return EXIT_FAILURE;
    }
    const auto *tbl = result->to<IR::P4Table>();
    auto tableName = tbl->controlPlaneName();

    if (!tableEntry.action().has_action()) {
        ::error("Table entry %1% has no action.", tableEntry.DebugString());
        return EXIT_FAILURE;
    }

    auto tblAction = tableEntry.action().action();
    auto actionId = tblAction.action_id();
    const auto *actionResult = irToIdMap.at(actionId);
    if (actionResult->to<IR::P4Action>() == nullptr) {
        ::error("Action %1% is not a IR::P4Action.", actionResult);
        return EXIT_FAILURE;
    }

    auto it = controlPlaneConstraints.find(tableName);
    if (it == controlPlaneConstraints.end()) {
        error(
            "Configuration for table %1% not found in the control plane constraints. It should "
            "have already been initialized at this point.",
            tableName);
        return EXIT_FAILURE;
    }

    auto &tableResultOpt = it->second.get();
    if (tableResultOpt.to<TableConfiguration>() == nullptr) {
        ::error("Configuration result is not a TableConfiguration.", tableName);
        return EXIT_FAILURE;
    }
    auto *tableResult = tableResultOpt.to<TableConfiguration>();

    const auto *p4Action = actionResult->to<IR::P4Action>();
    const auto *actionExpr = convertTableAction(tblAction, tableName, *p4Action);

    TableKeySet tableKeySet;
    for (const auto &field : tableEntry.match()) {
        auto fieldId = field.field_id();
        const auto *result = irToIdMap.at(P4::ControlPlaneAPI::szudzikPairing(tblId, fieldId));
        if (result->to<IR::KeyElement>() == nullptr) {
            ::error("%1% is not a IR::KeyElement.", result);
            return EXIT_FAILURE;
        }
        const auto *keyField = result->to<IR::KeyElement>();
        const auto *keyExpr = keyField->expression;
        const auto *nameAnnot = keyField->getAnnotation("name");
        if (nameAnnot == nullptr) {
            ::error("Non-constant table key without an annotation");
            return EXIT_FAILURE;
        }
        cstring keyFieldName;
        if (nameAnnot != nullptr) {
            keyFieldName = nameAnnot->getName();
        }
        auto matchSet = produceTableMatch(field, tableName, keyFieldName, *keyExpr);
        if (!matchSet.has_value()) {
            return EXIT_FAILURE;
        }
        tableKeySet.insert(matchSet.value().begin(), matchSet.value().end());
    }

    auto *tableMatchEntry = new TableMatchEntry(actionExpr, tableEntry.priority(), tableKeySet);
    tableResult->addTableEntry(*tableMatchEntry);
    return EXIT_SUCCESS;
}

int ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
    const p4::v1::Entity &entity, const P4RuntimeIdtoIrNodeMap &irToIdMap,
    ControlPlaneConstraints &controlPlaneConstraints) {
    if (entity.has_table_entry()) {
        const auto &tableEntry = entity.table_entry();
        if (convertTableEntry(irToIdMap, tableEntry, controlPlaneConstraints) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    } else {
        ::error("Unsupported control plane entry %1%.", entity.DebugString().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int ProtobufDeserializer::updateControlPlaneConstraints(
    const flaytests::Config &protoControlPlaneConfig, const P4RuntimeIdtoIrNodeMap &irToIdMap,
    ControlPlaneConstraints &controlPlaneConstraints) {
    for (const auto &entity : protoControlPlaneConfig.entities()) {
        if (updateControlPlaneConstraintsWithEntityMessage(
                entity, irToIdMap, controlPlaneConstraints) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

std::optional<p4::v1::Entity> ProtobufDeserializer::parseEntity(const std::string &message) {
    p4::v1::Entity entity;
    if (google::protobuf::TextFormat::ParseFromString(message, &entity)) {
        printInfo("Parsed entity: %1%", entity.DebugString());
    } else {
        ::error("Failed to parse Protobuf message: %1%", entity.ShortDebugString());
        return std::nullopt;
    }
    return entity;
}

}  // namespace P4Tools::Flay
