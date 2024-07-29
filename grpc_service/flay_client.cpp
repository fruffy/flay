#include "backends/p4tools/modules/flay/grpc_service/flay_client.h"

#include <fcntl.h>

#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "lib/error.h"
#include "lib/timer.h"
#include "p4/v1/p4runtime.pb.h"

namespace P4Tools::Flay {

FlayClient::FlayClient(const std::shared_ptr<grpc::Channel> &channel)
    : stub_(p4::v1::P4Runtime::NewStub(channel)) {}

grpc::Status FlayClient::sendWriteRequest(const p4::v1::WriteRequest &request) {
    printInfo("Sending update...\n");
    Util::ScopedTimer timer("sendGrpcWriteRequest");
    grpc::ClientContext context;
    p4::v1::WriteResponse response;
    return stub_->Write(&context, request, &response);
}

std::optional<p4::v1::WriteRequest> FlayClient::parseWriteRequestFile(
    const std::filesystem::path &inputFile) {
    p4::v1::WriteRequest request;

    // Parse the input file into the Protobuf object.
    int fd = open(inputFile.c_str(), O_RDONLY);
    google::protobuf::io::ZeroCopyInputStream *input =
        new google::protobuf::io::FileInputStream(fd);

    if (google::protobuf::TextFormat::Parse(input, &request)) {
        printInfo("Parsed configuration: %1%", request.DebugString());
    } else {
        ::error("WriteRequest not valid (partial content: %1%", request.ShortDebugString());
        return std::nullopt;
    }
    // Close the open file.
    close(fd);
    return request;
}

}  // namespace P4Tools::Flay
