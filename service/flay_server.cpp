#include "backends/p4tools/modules/flay/service/flay_server.h"

#include <cstdlib>
#include <optional>
#include <utility>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

class StatementCounter : public Inspector {
    uint64_t statementCount = 0;

 public:
    bool preorder(const IR::AssignmentStatement * /*statement*/) override {
        statementCount++;
        return false;
    }
    bool preorder(const IR::MethodCallStatement * /*statement*/) override {
        statementCount++;
        return false;
    }

    [[nodiscard]] uint64_t getStatementCount() const { return statementCount; }
};

uint64_t countStatements(const IR::P4Program *prog) {
    auto *counter = new StatementCounter();
    prog->apply(*counter);
    return counter->getStatementCount();
}

double measureProgramSize(const IR::P4Program *prog) {
    auto *programStream = new std::stringstream;
    auto *toP4 = new P4::ToP4(programStream, false);
    prog->apply(*toP4);
    return static_cast<double>(programStream->str().length());
}

double measureSizeDifference(const IR::P4Program *programBefore,
                             const IR::P4Program *programAfter) {
    double beforeLength = measureProgramSize(programBefore);

    return (beforeLength - measureProgramSize(programAfter)) * (100.0 / beforeLength);
}

FlayService::FlayService(const IR::P4Program *originalProgram,
                         const FlayCompilerResult &compilerResult, ReachabilityMap reachabilityMap,
                         AbstractSolver &solver,
                         ControlPlaneConstraints initialControlPlaneConstraints)
    : originalProgram(originalProgram),
      compilerResult(compilerResult),
      reachabilityMap(std::move(reachabilityMap)),
      solver(solver),
      controlPlaneConstraints(std::move(initialControlPlaneConstraints)) {
    originalProgram->apply(ReferenceResolver(refMap));
}

grpc::Status FlayService::Write(grpc::ServerContext * /*context*/,
                                const p4::v1::WriteRequest *request,
                                p4::v1::WriteResponse * /*response*/) {
    SymbolSet symbolSet;
    for (const auto &update : request->updates()) {
        Util::ScopedTimer timer("processMessage");
        if (ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
                update.entity(), getCompilerResult().getP4RuntimeNodeMap(), controlPlaneConstraints,
                update.type(), symbolSet) != EXIT_SUCCESS) {
            return {grpc::StatusCode::INTERNAL, "Failed to process update message"};
        }
    }
    if (elimControlPlaneDeadCode(symbolSet) != EXIT_SUCCESS) {
        return {grpc::StatusCode::INTERNAL, "Encountered problems while updating dead code."};
    }
    P4Tools::printPerformanceReport();
    return grpc::Status::OK;
}

void FlayService::printPrunedProgram() {
    P4::ToP4 toP4;
    prunedProgram->apply(toP4);
}

const IR::P4Program *FlayService::getPrunedProgram() const { return prunedProgram; }

const IR::P4Program *FlayService::getOriginalProgram() const { return originalProgram; }

const FlayCompilerResult &FlayService::getCompilerResult() const { return compilerResult.get(); }

int FlayService::elimControlPlaneDeadCode(
    std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) {
    Util::ScopedTimer timer("Eliminate Dead Code");

    std::optional<bool> hasChangedOpt;
    printInfo("Computing reachability...");
    if (symbolSet.has_value()) {
        hasChangedOpt = reachabilityMap.recomputeReachability(symbolSet.value(), solver,
                                                              controlPlaneConstraints);
    } else {
        hasChangedOpt = reachabilityMap.recomputeReachability(solver, controlPlaneConstraints);
    }
    if (!hasChangedOpt.has_value()) {
        return EXIT_FAILURE;
    }
    bool hasChanged = hasChangedOpt.value();
    if (!hasChanged) {
        printInfo("Received update, but semantics have not changed. No program change necessary.");
        return EXIT_SUCCESS;
    }
    printInfo("Dead code that can be removed detected.");
    semanticsChangeCounter++;

    prunedProgram = originalProgram->apply(ElimDeadCode(refMap, reachabilityMap));
    auto statementCountBefore = countStatements(originalProgram);
    auto statementCountAfter = countStatements(prunedProgram);
    float stmtPct = 100.0F * (1.0F - static_cast<float>(statementCountAfter) /
                                         static_cast<float>(statementCountBefore));
    printInfo("Number of statements - Before: %1% After: %2% Total reduction percentage = %3%%%",
              statementCountBefore, statementCountAfter, stmtPct);
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
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

    auto f = exit_requested.get_future();
    f.wait();
    server->Shutdown();
    servingThread.join();
    return true;
}

}  // namespace P4Tools::Flay
