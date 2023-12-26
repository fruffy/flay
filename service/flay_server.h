#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_

#include <grpcpp/grpcpp.h>

#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/core/reachability.h"
#include "ir/solver.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "control-plane/p4/v1/p4runtime.grpc.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/core/compiler_target.h"

namespace P4Tools::Flay {

class FlayService final : public p4::v1::P4Runtime::Service {
 private:
    /// The original source P4 program.
    const IR::P4Program *originalProgram;

    /// The P4 program after pruning. Should be initialized with the initial data plane
    /// configuration
    const IR::P4Program *prunedProgram = nullptr;

    /// The program info object stores the results of the compilation, which includes the P4 program
    /// and any information extracted from the program using static analysis.
    std::reference_wrapper<const FlayCompilerResult> compilerResult;

    /// The reachability map.
    ReachabilityMap reachabilityMap;

    /// The constraint solver associated with the tool. Currently specialized to Z3.
    std::reference_wrapper<AbstractSolver> solver;

    /// The set of active control plane constraints. These constraints are added to every solver
    /// check to compute feasibility of a program node.
    ControlPlaneConstraints controlPlaneConstraints;

 public:
    explicit FlayService(const IR::P4Program *originalProgram,
                         const FlayCompilerResult &compilerResult, ReachabilityMap reachabilityMap,
                         AbstractSolver &solver,
                         ControlPlaneConstraints initialControlPlaneConstraints);

    /// Start the Flay gRPC server and listen to incoming requests.
    bool startServer(const std::string &serverAddress);

    /// Process an incoming gRPC request. This is typically a P4Runtime control plane update.
    grpc::Status Write(grpc::ServerContext * /*context*/, const p4::v1::WriteRequest *request,
                       p4::v1::WriteResponse * /*response*/) override;

    /// Print the pruned program to stdout;
    void printPrunedProgram();

    /// Process a P4Runtime INSERT or MODIFY message.
    [[nodiscard]] grpc::Status processUpdateMessage(const p4::v1::Entity &entity);

    /// Process a P4Runtime DELETE message.
    [[nodiscard]] grpc::Status processDeleteMessage(const p4::v1::Entity &entity);

    /// @returns the pruned program
    [[nodiscard]] const IR::P4Program *getPrunedProgram() const;

    /// @returns the original program
    [[nodiscard]] const IR::P4Program *getOriginalProgram() const;

    /// @returns a reference to the compiler result that this program info object was initialized
    /// with.
    [[nodiscard]] const FlayCompilerResult &getCompilerResult() const;

    /// Add new control plane constraints to the service.
    void addControlPlaneConstraints(const ControlPlaneConstraints &newControlPlaneConstraints);

    /// Remove control plane constraints from the service.
    void removeControlPlaneConstraints(const ControlPlaneConstraints &newControlPlaneConstraints);

    /// Run dead code elimination on the original P4 program.
    const IR::P4Program *elimControlPlaneDeadCode();
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_
