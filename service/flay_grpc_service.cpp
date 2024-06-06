#include "backends/p4tools/modules/flay/service/flay_grpc_service.h"

#include <glob.h>

#include <cstdlib>
#include <utility>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/p4runtime/protobuf.h"
#include "backends/p4tools/modules/flay/core/flay_service.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

FlayService::FlayService(const FlayServiceOptions &options,
                         const FlayCompilerResult &compilerResult,
                         const NodeAnnotationMap &nodeAnnotationMap,
                         ControlPlaneConstraints initialControlPlaneConstraints)
    : FlayServiceBase(options, compilerResult, nodeAnnotationMap,
                      std::move(initialControlPlaneConstraints)) {}

grpc::Status FlayService::Write(grpc::ServerContext * /*context*/,
                                const p4::v1::WriteRequest *request,
                                p4::v1::WriteResponse * /*response*/) {
    SymbolSet symbolSet;
    for (const auto &update : request->updates()) {
        Util::ScopedTimer timer("processMessage");
        auto result = P4Runtime::updateControlPlaneConstraintsWithEntityMessage(
            update.entity(), *compilerResult().getP4RuntimeApi().p4Info,
            mutableControlPlaneConstraints(), update.type(), symbolSet);
        if (result != EXIT_SUCCESS) {
            return {grpc::StatusCode::INTERNAL, "Failed to process update message"};
        }
    }
    auto result = specializeProgram(symbolSet);
    if (result.first != EXIT_SUCCESS) {
        return {grpc::StatusCode::INTERNAL, "Encountered problems while updating dead code."};
    }
    auto statementCountBefore = countStatements(originalProgram());
    auto statementCountAfter = countStatements(optimizedProgram());
    float stmtPct = 100.0F * (1.0F - static_cast<float>(statementCountAfter) /
                                         static_cast<float>(statementCountBefore));
    printInfo("Number of statements - Before: %1% After: %2% Total reduction in statements = %3%%%",
              statementCountBefore, statementCountAfter, stmtPct);

    P4Tools::printPerformanceReport();
    return grpc::Status::OK;
}

bool FlayService::startServer(const std::string &serverAddress) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(this);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        ::error("Failed to start the Flay service.");
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

}  // namespace P4Tools::Flay
