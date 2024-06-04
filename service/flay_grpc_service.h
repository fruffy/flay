#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_GRPC_SERVICE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_GRPC_SERVICE_H_
#include <grpcpp/grpcpp.h>

#include <future>

#include "backends/p4tools/modules/flay/core/flay_service.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "control-plane/p4/v1/p4runtime.grpc.pb.h"
#pragma GCC diagnostic pop

namespace P4Tools::Flay {

class FlayService final : public FlayServiceBase, public p4::v1::P4Runtime::Service {
    /// For exiting the gRPC server (useful for benchmarking).
    std::promise<void> exitRequested;

 public:
    explicit FlayService(const FlayServiceOptions &options,
                         const FlayCompilerResult &compilerResult,
                         const NodeAnnotationMap &nodeAnnotationMap,
                         ControlPlaneConstraints initialControlPlaneConstraints);

    /// Start the Flay gRPC server and listen to incoming requests.
    bool startServer(const std::string &serverAddress);

    /// Process an incoming gRPC request. This is typically a P4Runtime control plane update.
    grpc::Status Write(grpc::ServerContext * /*context*/, const p4::v1::WriteRequest *request,
                       p4::v1::WriteResponse * /*response*/) override;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_GRPC_SERVICE_H_
