#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"

#include <fcntl.h>

#include <google/protobuf/text_format.h>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

flaytests::Config ProtobufDeserializer::deserializeProtobufConfig(std::filesystem::path inputFile) {
    flaytests::Config protoControlPlaneConfig;

    // Parse the input file into the Protobuf object.
    int fd = open(inputFile.c_str(), O_RDONLY);
    google::protobuf::io::ZeroCopyInputStream *input =
        new google::protobuf::io::FileInputStream(fd);

    if (google::protobuf::TextFormat::Parse(input, &protoControlPlaneConfig)) {
        std::cout << "Parse configuration: " << protoControlPlaneConfig.DebugString();
    } else {
        std::cerr << "Message not valid (partial content: "
                  << protoControlPlaneConfig.ShortDebugString() << ")\n";
    }
    // Close the open file.
    close(fd);
    return protoControlPlaneConfig;
}

void ProtobufDeserializer::convertTableMatch(const p4::v1::FieldMatch &field, cstring tableName,
                                             cstring keyFieldName, const IR::Expression &keyExpr,
                                             ControlPlaneConstraints &controlPlaneConstraints) {
    if (field.has_exact()) {
        cstring keyName = tableName + "_key_" + keyFieldName;
        auto keySymbol = ToolsVariables::getSymbolicVariable(keyExpr.type, keyName);
        big_int value;
        auto fieldString = field.exact().value();
        boost::multiprecision::import_bits(value, fieldString.begin(), fieldString.end());
        auto assign = new IR::Equ(keySymbol, new IR::Constant(keyExpr.type, value));
        controlPlaneConstraints.emplace_back(assign);
    } else {
        P4C_UNIMPLEMENTED("Unsupported table match type %1%.", field.DebugString().c_str());
    }
}

void ProtobufDeserializer::convertTableAction(const p4::v1::Action &tblAction, cstring tableName,
                                              const IR::P4Action &p4Action,
                                              ControlPlaneConstraints &controlPlaneConstraints) {
    auto actionVarName = tableName + "_action";
    const auto *tableActionID =
        ToolsVariables::getSymbolicVariable(new IR::Type_String, actionVarName);

    auto actionName = p4Action.controlPlaneName();
    const auto *actionAssignment = new IR::StringLiteral(actionName);
    controlPlaneConstraints.emplace_back(new IR::Equ(tableActionID, actionAssignment));
    for (auto paramConfig : tblAction.params()) {
        auto param = p4Action.parameters->getParameter(paramConfig.param_id() - 1);
        auto paramName = param->controlPlaneName();
        cstring paramLabel = tableName + "_" + actionName + "_" + paramName;
        const auto &actionArg = ToolsVariables::getSymbolicVariable(param->type, paramLabel);
        big_int value;
        auto fieldString = paramConfig.value();
        boost::multiprecision::import_bits(value, fieldString.begin(), fieldString.end());
        const auto *actionVal = new IR::Constant(param->type, value);
        controlPlaneConstraints.emplace_back(new IR::Equ(actionArg, actionVal));
    }
}

void ProtobufDeserializer::convertTableEntry(const P4RuntimeIdtoIrNodeMap &irToIdMap,
                                             const p4::v1::TableEntry &tableEntry,
                                             ControlPlaneConstraints &controlPlaneConstraints) {
    auto tblId = tableEntry.table_id();
    auto tbl = irToIdMap.at(tblId)->checkedTo<IR::P4Table>();
    auto tableName = tbl->controlPlaneName();

    auto key = tbl->getKey();
    CHECK_NULL(key);
    for (auto field : tableEntry.match()) {
        auto fieldId = field.field_id();
        auto keyField = key->keyElements.at(fieldId - 1);
        const auto *keyExpr = keyField->expression;
        const auto *nameAnnot = keyField->getAnnotation("name");
        // Some hidden tables do not have any key name annotations.
        BUG_CHECK(nameAnnot != nullptr /* || properties.tableIsImmutable*/,
                  "Non-constant table key without an annotation");
        cstring keyFieldName;
        if (nameAnnot != nullptr) {
            keyFieldName = nameAnnot->getName();
        }
        convertTableMatch(field, tableName, keyFieldName, *keyExpr, controlPlaneConstraints);
    }

    if (tableEntry.action().has_action()) {
        auto tblAction = tableEntry.action().action();
        auto actionId = tblAction.action_id();
        auto p4Action = irToIdMap.at(actionId)->checkedTo<IR::P4Action>();
        convertTableAction(tblAction, tableName, *p4Action, controlPlaneConstraints);
    }
}
ControlPlaneConstraints ProtobufDeserializer::convertToControlPlaneConstraints(
    const flaytests::Config &protoControlPlaneConfig, const P4RuntimeIdtoIrNodeMap &irToIdMap) {
    ControlPlaneConstraints controlPlaneConstraints;
    for (auto entity : protoControlPlaneConfig.entities()) {
        if (entity.has_table_entry()) {
            auto tableEntry = entity.table_entry();
            convertTableEntry(irToIdMap, tableEntry, controlPlaneConstraints);
        } else {
            P4C_UNIMPLEMENTED("Unsupported control plane entry %1%.", entity.DebugString().c_str());
        }
    }
    return controlPlaneConstraints;
}

}  // namespace P4Tools::Flay
