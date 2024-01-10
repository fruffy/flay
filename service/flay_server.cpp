#include "backends/p4tools/modules/flay/service/flay_server.h"

#include <glob.h>

#include <cstdlib>
#include <optional>
#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/core/z3solver_reachability.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/error.h"
#include "lib/timer.h"
#include "p4/v1/p4runtime.pb.h"

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

AbstractReachabilityMap &FlayService::initializeReachabilityMap(
    ReachabilityMapType mapType, const ReachabilityMap &reachabilityMap) {
    printInfo("Creating the reachability map...");
    AbstractReachabilityMap *initializedReachabilityMap = nullptr;
    if (mapType == ReachabilityMapType::Z3_PRECOMPUTED) {
        initializedReachabilityMap = new Z3SolverReachabilityMap(reachabilityMap);
    } else {
        auto *solver = new Z3Solver();
        initializedReachabilityMap = new SolverReachabilityMap(*solver, reachabilityMap);
    }
    return *initializedReachabilityMap;
}

FlayService::FlayService(const FlayServiceOptions &options, const IR::P4Program *originalProgram,
                         const FlayCompilerResult &compilerResult,
                         const ReachabilityMap &reachabilityMap,
                         ControlPlaneConstraints initialControlPlaneConstraints)
    : options(options),
      originalProgram(originalProgram),
      compilerResult(compilerResult),
      reachabilityMap(initializeReachabilityMap(options.mapType, reachabilityMap)),
      controlPlaneConstraints(std::move(initialControlPlaneConstraints)) {
    printInfo("Checking whether dead code can be removed with the initial configuration...");
    originalProgram->apply(ReferenceResolver(refMap));
    elimControlPlaneDeadCode();
}

int FlayService::updateControlPlaneConstraintsWithEntityMessage(
    const p4::v1::Entity &entity, const ::p4::v1::Update_Type &updateType, SymbolSet &symbolSet) {
    return ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
        entity, getCompilerResult().getP4RuntimeNodeMap(), controlPlaneConstraints, updateType,
        symbolSet);
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
    auto result = elimControlPlaneDeadCode(symbolSet);
    if (result.first != EXIT_SUCCESS) {
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

std::optional<bool> FlayService::checkForSemanticChange(
    std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) const {
    printInfo("Checking for change in reachability semantics...");
    Util::ScopedTimer timer("Check for semantics change");
    if (symbolSet.has_value() && options.useSymbolSet) {
        return reachabilityMap.get().recomputeReachability(symbolSet.value(),
                                                           controlPlaneConstraints);
    }
    return reachabilityMap.get().recomputeReachability(controlPlaneConstraints);
}

std::pair<int, bool> FlayService::elimControlPlaneDeadCode(
    std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) {
    Util::ScopedTimer timer("Eliminate Dead Code");

    std::optional<bool> hasChangedOpt = checkForSemanticChange(symbolSet);
    if (!hasChangedOpt.has_value()) {
        return {EXIT_FAILURE, false};
    }
    bool hasChanged = hasChangedOpt.value();
    if (!hasChanged) {
        printInfo("Received update, but semantics have not changed. No program change necessary.");
        return {EXIT_SUCCESS, hasChanged};
    }
    printInfo("Change in semantics detected.");

    prunedProgram = originalProgram->apply(ElimDeadCode(refMap, reachabilityMap));
    return ::errorCount() == 0 ? std::pair{EXIT_SUCCESS, hasChanged}
                               : std::pair{EXIT_FAILURE, hasChanged};
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

std::vector<std::string> FlayServiceWrapper::findFiles(const std::string &pattern) {
    std::vector<std::string> files;
    glob_t globResult;

    // Perform globbing.
    if (glob(pattern.c_str(), GLOB_TILDE, nullptr, &globResult) == 0) {
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            files.emplace_back(globResult.gl_pathv[i]);
        }
    }

    // Free allocated resources
    globfree(&globResult);

    return files;
}

int FlayServiceWrapper::parseControlUpdatesFromPattern(const std::string &pattern) {
    auto files = findFiles(pattern);
    for (const auto &file : files) {
        auto entityOpt =
            ProtobufDeserializer::deserializeProtoObjectFromFile<p4::v1::WriteRequest>(file);
        if (!entityOpt.has_value()) {
            return EXIT_FAILURE;
        }
        controlPlaneUpdates.emplace_back(entityOpt.value());
    }
    return EXIT_SUCCESS;
}

void FlayServiceWrapper::recordProgramChange(const FlayService &service) {
    auto statementCountBefore = countStatements(service.originalProgram);
    auto statementCountAfter = countStatements(service.prunedProgram);
    float stmtPct = 100.0F * (1.0F - static_cast<float>(statementCountAfter) /
                                         static_cast<float>(statementCountBefore));
    printInfo("Number of statements - Before: %1% After: %2% Total reduction in statements = %3%%%",
              statementCountBefore, statementCountAfter, stmtPct);
}

int FlayServiceWrapper::run(const FlayServiceOptions &serviceOptions,
                            const IR::P4Program *originalProgram,
                            const FlayCompilerResult &compilerResult,
                            const ReachabilityMap &reachabilityMap,
                            const ControlPlaneConstraints &initialControlPlaneConstraints) const {
    // Initialize the flay service, which includes a dead code eliminator.
    FlayService service(serviceOptions, originalProgram, compilerResult, reachabilityMap,
                        initialControlPlaneConstraints);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }
    recordProgramChange(service);

    /// Keeps track of how often the semantics have changed after an update.
    uint64_t semanticsChangeCounter = 0;
    if (!controlPlaneUpdates.empty()) {
        printInfo("Processing control plane updates...");
    }
    for (const auto &controlPlaneUpdate : controlPlaneUpdates) {
        SymbolSet symbolSet;
        for (const auto &update : controlPlaneUpdate.updates()) {
            Util::ScopedTimer timer("processMessage");
            if (service.updateControlPlaneConstraintsWithEntityMessage(
                    update.entity(), update.type(), symbolSet) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
        }
        auto result = service.elimControlPlaneDeadCode(symbolSet);
        if (result.first != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        if (result.second) {
            recordProgramChange(service);
            semanticsChangeCounter++;
        }
    }

    return EXIT_SUCCESS;
}

}  // namespace P4Tools::Flay
