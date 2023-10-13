#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_

#include <filesystem>

#include "ir/ir.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/modules/flay/control_plane/protobuf/flaytests.pb.h"
#pragma GCC diagnostic pop

namespace P4Tools::Flay {

class ProtobufDeserializer {
 public:
    static flaytests::Config deserializeProtobufConfig(std::filesystem::path inputFile);

    static std::vector<const IR::Expression *> convertToIRExpressions(
        const flaytests::Config &protoControlPlaneConfig);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_ */
