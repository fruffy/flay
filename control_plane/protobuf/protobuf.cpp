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

ControlPlaneConstraints ProtobufDeserializer::convertToControlPlaneConstraints(
    const flaytests::Config &protoControlPlaneConfig, const P4RuntimeIDtoIRObjectMap &irToIdMap) {
    ControlPlaneConstraints controlPlaneConstraints;
    for (auto entity : protoControlPlaneConfig.entities()) {
        if (entity.has_table_entry()) {
            auto tblEntry = entity.table_entry();
            auto tblId = tblEntry.table_id();
            auto tbl = irToIdMap.at(tblId)->checkedTo<IR::P4Table>();
            auto tableName = tbl->controlPlaneName();

            auto key = tbl->getKey();
            CHECK_NULL(key);
            for (auto field : tblEntry.match()) {
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
                if (field.has_exact()) {
                    cstring keyName = tableName + "_key_" + keyFieldName;
                    auto keySymbol = ToolsVariables::getSymbolicVariable(keyExpr->type, keyName);
                    big_int value;
                    auto fieldString = field.exact().value();
                    boost::multiprecision::import_bits(value, fieldString.begin(),
                                                       fieldString.end());
                    auto assign = new IR::Equ(keySymbol, new IR::Constant(keyExpr->type, value));
                    controlPlaneConstraints.emplace_back(assign);
                }
            }

            if (tblEntry.action().has_action()) {
                auto actionVarName = tableName + "_action";
                const auto *tableActionID =
                    ToolsVariables::getSymbolicVariable(new IR::Type_String, actionVarName);
                auto tblAction = tblEntry.action().action();
                auto actionId = tblAction.action_id();
                auto action = irToIdMap.at(actionId)->checkedTo<IR::P4Action>();
                auto actionName = action->controlPlaneName();
                const auto *actionAssignment = new IR::StringLiteral(actionName);
                controlPlaneConstraints.emplace_back(new IR::Equ(tableActionID, actionAssignment));
                for (auto paramConfig : tblAction.params()) {
                    auto param = action->parameters->getParameter(paramConfig.param_id() - 1);
                    auto paramName = param->controlPlaneName();
                    cstring paramLabel = tableName + "_" + actionName + "_" + paramName;
                    const auto &actionArg =
                        ToolsVariables::getSymbolicVariable(param->type, paramLabel);
                    big_int value;
                    auto fieldString = paramConfig.value();
                    boost::multiprecision::import_bits(value, fieldString.begin(),
                                                       fieldString.end());
                    const auto *actionVal = new IR::Constant(param->type, value);
                    controlPlaneConstraints.emplace_back(new IR::Equ(actionArg, actionVal));
                }
            } else {
                P4C_UNIMPLEMENTED("Unsupported control plane entry %1%.",
                                  entity.DebugString().c_str());
            }
        }
    }
    return controlPlaneConstraints;
}

}  // namespace P4Tools::Flay
