#include "backends/p4tools/modules/flay/service/flay_server.h"

#include <utility>

#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/lib/logging.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

FlayService::FlayService(const IR::P4Program *originalProgram, const ExecutionState &executionState,
                         AbstractSolver &solver, P4RuntimeIdtoIrNodeMap idToIrMap)
    : originalProgram(originalProgram),
      idToIrMap(std::move(idToIrMap)),
      elimDeadCode(executionState, solver) {}

grpc::Status FlayService::Write(grpc::ServerContext * /*context*/,
                                const p4::v1::WriteRequest *request,
                                p4::v1::WriteResponse * /*response*/) {
    for (const auto &update : request->updates()) {
        if (update.type() == p4::v1::Update::INSERT || update.type() == p4::v1::Update::MODIFY) {
            processUpdateMessage(update.entity());
        } else if (update.type() == p4::v1::Update::DELETE) {
            processDeleteMessage(update.entity());
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

void FlayService::processUpdateMessage(const p4::v1::Entity &entity) {
    Util::ScopedTimer timer("processUpdateMessage");
    auto constraints = ProtobufDeserializer::convertEntityMessageToConstraints(entity, idToIrMap);
    elimDeadCode.addControlPlaneConstraints(constraints);
    elimControlPlaneDeadCode();
}

void FlayService::processDeleteMessage(const p4::v1::Entity &entity) {
    Util::ScopedTimer timer("processDeleteMessage");
    auto constraints = ProtobufDeserializer::convertEntityMessageToConstraints(entity, idToIrMap);
    elimDeadCode.removeControlPlaneConstraints(constraints);
    elimControlPlaneDeadCode();
}

const IR::P4Program *FlayService::getPrunedProgram() const { return prunedProgram; }

const IR::P4Program *FlayService::getOriginalProgram() const { return originalProgram; }

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
