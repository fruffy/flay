#include "backends/p4tools/modules/flay/service/flay_server.h"

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
                         const FlayCompilerResult &compilerResult,
                         const ExecutionState &executionState, AbstractSolver &solver,
                         ControlPlaneConstraints &initialControlPlaneConstraints)
    : originalProgram(originalProgram),
      compilerResult(compilerResult),
      elimDeadCode(executionState, solver) {
    elimDeadCode.addControlPlaneConstraints(initialControlPlaneConstraints);
}

grpc::Status FlayService::Write(grpc::ServerContext * /*context*/,
                                const p4::v1::WriteRequest *request,
                                p4::v1::WriteResponse * /*response*/) {
    for (const auto &update : request->updates()) {
        if (update.type() == p4::v1::Update::INSERT || update.type() == p4::v1::Update::MODIFY) {
            auto status = processUpdateMessage(update.entity());
            if (!status.ok()) {
                return status;
            }
        } else if (update.type() == p4::v1::Update::DELETE) {
            auto status = processDeleteMessage(update.entity());
            if (!status.ok()) {
                return status;
            }
        } else {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Unknown update type"};
        }
    }

    return grpc::Status::OK;
}

void FlayService::printPrunedProgram() {
    P4::ToP4 toP4;
    prunedProgram->apply(toP4);
}

grpc::Status FlayService::processUpdateMessage(const p4::v1::Entity &entity) {
    Util::ScopedTimer timer("processUpdateMessage");
    // TODO: Clean up this interface.
    auto constraints = elimDeadCode.getWriteableControlPlaneConstraints();
    if (!ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
            entity, getCompilerResult().getP4RuntimeNodeMap(), constraints)) {
        return {grpc::StatusCode::INTERNAL, "Failed to process update message"};
    }
    elimControlPlaneDeadCode();
    if (::errorCount() > 0) {
        return {grpc::StatusCode::INTERNAL,
                "processUpdateMessage: Encountered problems while updating dead code."};
    }
    P4Tools::printPerformanceReport();
    return grpc::Status::OK;
}

grpc::Status FlayService::processDeleteMessage(const p4::v1::Entity &entity) {
    Util::ScopedTimer timer("processDeleteMessage");
    // TODO: Clean up this interface.
    // This does not handle delete correctly.
    auto constraints = elimDeadCode.getWriteableControlPlaneConstraints();
    if (!ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
            entity, getCompilerResult().getP4RuntimeNodeMap(), constraints)) {
        return {grpc::StatusCode::INTERNAL, "Failed to process delete message"};
    }
    elimControlPlaneDeadCode();
    if (::errorCount() > 0) {
        return {grpc::StatusCode::INTERNAL,
                "processDeleteMessage: Encountered problems while updating dead code."};
    }

    P4Tools::printPerformanceReport();
    return grpc::Status::OK;
}

const IR::P4Program *FlayService::getPrunedProgram() const { return prunedProgram; }

const IR::P4Program *FlayService::getOriginalProgram() const { return originalProgram; }

const FlayCompilerResult &FlayService::getCompilerResult() const { return compilerResult.get(); }

void FlayService::addControlPlaneConstraints(
    const ControlPlaneConstraints &newControlPlaneConstraints) {
    elimDeadCode.addControlPlaneConstraints(newControlPlaneConstraints);
}

void FlayService::removeControlPlaneConstraints(
    const ControlPlaneConstraints &newControlPlaneConstraints) {
    elimDeadCode.removeControlPlaneConstraints(newControlPlaneConstraints);
}

const IR::P4Program *FlayService::elimControlPlaneDeadCode() {
    Util::ScopedTimer timer("Eliminate Dead Code");
    prunedProgram = originalProgram->apply(elimDeadCode);
    printInfo("Number of statements - Before: %1% After: %2% Total reduction percentage = %3%%%",
              countStatements(originalProgram), countStatements(prunedProgram),
              measureSizeDifference(originalProgram, prunedProgram));
    return prunedProgram;
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

    printInfo("Server listening on: %1%", serverAddress);
    server->Wait();
    return true;
}

}  // namespace P4Tools::Flay
