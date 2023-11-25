#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_

#include <grpcpp/grpcpp.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "control-plane/p4/v1/p4runtime.grpc.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

namespace P4Tools::Flay {

class FlayService final : public p4::v1::P4Runtime::Service {
 private:
    /// The original source P4 program.
    const IR::P4Program *originalProgram;

    /// The P4 program after pruning. Should be initialized with the initial data plane
    /// configuration
    const IR::P4Program *prunedProgram = nullptr;

    /// A mapping of P4Runtime IDs to the corresponding IR nodes.
    P4RuntimeIdtoIrNodeMap idToIrMap;

    /// The dead code eliminator pass.
    ElimDeadCode elimDeadCode;

 public:
    explicit FlayService(const IR::P4Program *originalProgram, const ExecutionState &executionState,
                         AbstractSolver &solver, P4RuntimeIdtoIrNodeMap idToIrMap);

    /// Start the Flay gRPC server and listen to incoming requests.
    bool startServer(const std::string &serverAddress);

    /// Process an incoming gRPC request. This is typically a P4Runtime control plane update.
    grpc::Status Write(grpc::ServerContext * /*context*/, const p4::v1::WriteRequest *request,
                       p4::v1::WriteResponse * /*response*/) override;

    /// Print the pruned program to stdout;
    void printPrunedProgram();

    /// Process a P4Runtime INSERT or MODIFY message.
    void processUpdateMessage(const p4::v1::Entity &entity);

    /// Process a P4Runtime DELETE message.
    void processDeleteMessage(const p4::v1::Entity &entity);

    /// @returns the pruned program
    const IR::P4Program *getPrunedProgram() const;

    /// @returns the original program
    const IR::P4Program *getOriginalProgram() const;

    /// Add new control plane constraints to the service.
    /// TODO: Implement corresponding removal function.
    void addControlPlaneConstraints(const ControlPlaneConstraints &newControlPlaneConstraints);

    /// Run dead code elimination on the original P4 program.
    const IR::P4Program *elimControlPlaneDeadCode();
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_
