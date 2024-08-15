#include "backends/p4tools/modules/flay/grpc_service/flay_grpc_service.h"

#include <glob.h>

#include <cstdlib>
#include <utility>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/control_plane/p4runtime/protobuf.h"
#include "backends/p4tools/modules/flay/core/specialization/flay_service.h"
#include "lib/timer.h"

namespace P4::P4Tools::Flay {

FlayService::FlayService(const FlayCompilerResult &compilerResult,
                         IncrementalAnalysisMap incrementalAnalysisMap)
    : FlayServiceBase(compilerResult, std::move(incrementalAnalysisMap)) {}

grpc::Status FlayService::Write(grpc::ServerContext * /*context*/,
                                const p4::v1::WriteRequest *request,
                                p4::v1::WriteResponse * /*response*/) {
    SymbolSet symbolSet;
    std::vector<const ControlPlaneUpdate *> p4RuntimeUpdates;
    for (const auto &update : request->updates()) {
        p4RuntimeUpdates.emplace_back(new P4RuntimeControlPlaneUpdate(update));
    }
    auto result = processControlPlaneUpdate(p4RuntimeUpdates);
    if (result != EXIT_SUCCESS) {
        return {grpc::StatusCode::INTERNAL, "Failed to process update message"};
    }
    recordProgramChange();
    return grpc::Status::OK;
}

bool FlayService::startServer(const std::string &serverAddress) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(this);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        ::P4::error("Failed to start the Flay service.");
        return false;
    }

    printInfo("Flay service listening on: %1%", serverAddress);

    auto serveFn = [&]() { server->Wait(); };
    std::thread servingThread(serveFn);

    auto f = exitRequested.get_future();
    f.wait();
    server->Shutdown();
    servingThread.join();
    return true;
}

}  // namespace P4::P4Tools::Flay
