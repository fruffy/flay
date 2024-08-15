#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_GRPC_SERVICE_FLAY_CLIENT_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_GRPC_SERVICE_FLAY_CLIENT_H_

#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include <filesystem>
#include <memory>
#include <optional>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "control-plane/p4/v1/p4runtime.grpc.pb.h"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::Flay {

class FlayClient {
 public:
    explicit FlayClient(const std::shared_ptr<grpc::Channel> &channel);

    /// Parse a text Protobuf which describes a write request.
    static std::optional<p4::v1::WriteRequest> parseWriteRequestFile(
        const std::filesystem::path &inputFile);

    /// Send a write request to the gRPC server.
    /// @returns grpc::StatusOK if everything went well.
    [[nodiscard]] grpc::Status sendWriteRequest(const p4::v1::WriteRequest &request);

 private:
    std::unique_ptr<p4::v1::P4Runtime::Stub> stub_;
};

}  // namespace P4::P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_GRPC_SERVICE_FLAY_CLIENT_H_
