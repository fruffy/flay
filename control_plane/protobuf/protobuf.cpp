#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"

#include <fcntl.h>

#include <google/protobuf/text_format.h>

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

}  // namespace P4Tools::Flay
