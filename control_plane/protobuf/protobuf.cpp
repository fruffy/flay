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

std::vector<const IR::Expression *> ProtobufDeserializer::convertToIRExpressions(
    const flaytests::Config &protoControlPlaneConfig) {
    std::vector<const IR::Expression *> irExpressions;
    for (auto entity : protoControlPlaneConfig.entities()) {
        if (entity.has_table_entry()) {
            auto tblEntry = entity.table_entry();
            auto tblName = tblEntry.table_id();

            if (tblEntry.action().has_action()) {
                auto actionVarName = std::to_string(tblName) + "_action";
                const auto *tableActionID =
                    ToolsVariables::getSymbolicVariable(new IR::Type_String, actionVarName);
                auto tblAction = tblEntry.action().action();
                auto actionName = std::to_string(tblAction.action_id());
                const auto *actionAssignment = new IR::StringLiteral(actionName);
                irExpressions.push_back(new IR::Equ(tableActionID, actionAssignment));
                for (auto param : tblAction.params()) {
                    cstring paramName = std::to_string(tblName) + "_" + actionName + "_" +
                                        std::to_string(param.param_id());
                    const auto *argType = IR::getBitType(12, 0);
                    const auto &actionArg = ToolsVariables::getSymbolicVariable(argType, paramName);
                    const auto *actionVal = new IR::Constant(argType, big_int(param.value()));
                    irExpressions.push_back(new IR::Equ(actionArg, actionVal));
                }
            } else {
                P4C_UNIMPLEMENTED("Unsupported control plane entry %1%.",
                                  entity.DebugString().c_str());
            }
        }
    }
    return irExpressions;
}

}  // namespace P4Tools::Flay
