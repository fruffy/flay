#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_

#include <grpcpp/grpcpp.h>

#include <functional>
#include <future>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "control-plane/p4/v1/p4runtime.grpc.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/core/compiler_target.h"
#include "backends/p4tools/modules/flay/core/reachability.h"

namespace P4Tools::Flay {

enum ReachabilityMapType { Z3_PRECOMPUTED, Z3_DEFAULT };

struct FlayServiceOptions {
    /// If useSymbolSet is true, we only check whether the symbols in the set have changed.
    bool useSymbolSet = false;
    /// The type of map to initialize.
    ReachabilityMapType mapType = ReachabilityMapType::Z3_PRECOMPUTED;
};

class FlayService final : public p4::v1::P4Runtime::Service {
    friend class FlayServiceWrapper;

 private:
    // Configuration options for the service.
    FlayServiceOptions options;

    /// The original source P4 program.
    const IR::P4Program *originalProgram;

    /// The P4 program after pruning. Should be initialized with the initial data plane
    /// configuration
    const IR::P4Program *prunedProgram = nullptr;

    /// The program info object stores the results of the compilation, which includes the P4 program
    /// and any information extracted from the program using static analysis.
    std::reference_wrapper<const FlayCompilerResult> compilerResult;

    /// The reachability map used by the server. Derived from the input argument.
    std::reference_wrapper<AbstractReachabilityMap> reachabilityMap;

    /// The set of active control plane constraints. These constraints are added to every solver
    /// check to compute feasibility of a program node.
    ControlPlaneConstraints controlPlaneConstraints;

    /// A map to look up declaration references.
    P4::ReferenceMap refMap;

    /// For exiting the gRPC server (useful for benchmarking).
    std::promise<void> exitRequested;

    /// Compute whether the semantics of the program under this control plane configuration have
    /// changed since the last update.
    // @returns std::nullopt if an error occurred during the computation.
    std::optional<bool> checkForSemanticChange(
        std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) const;

    /// Update the internal  control plane constraints with the given entity message.
    /// Private method used for testing.
    int updateControlPlaneConstraintsWithEntityMessage(const p4::v1::Entity &entity,
                                                       const ::p4::v1::Update_Type &updateType,
                                                       SymbolSet &symbolSet);

 public:
    explicit FlayService(const FlayServiceOptions &options, const IR::P4Program *originalProgram,
                         const FlayCompilerResult &compilerResult,
                         const ReachabilityMap &reachabilityMap,
                         ControlPlaneConstraints initialControlPlaneConstraints);

    /// Start the Flay gRPC server and listen to incoming requests.
    bool startServer(const std::string &serverAddress);

    /// Process an incoming gRPC request. This is typically a P4Runtime control plane update.
    grpc::Status Write(grpc::ServerContext * /*context*/, const p4::v1::WriteRequest *request,
                       p4::v1::WriteResponse * /*response*/) override;

    /// Print the pruned program to stdout;
    void printPrunedProgram();

    static AbstractReachabilityMap &initializeReachabilityMap(
        ReachabilityMapType mapType, const ReachabilityMap &reachabilityMap);

    /// @returns the pruned program
    [[nodiscard]] const IR::P4Program *getPrunedProgram() const;

    /// @returns the original program
    [[nodiscard]] const IR::P4Program *getOriginalProgram() const;

    /// @returns a reference to the compiler result that this program info object was initialized
    /// with.
    [[nodiscard]] const FlayCompilerResult &getCompilerResult() const;

    /// Run dead code elimination on the original P4 program.
    std::pair<int, bool> elimControlPlaneDeadCode(
        std::optional<std::reference_wrapper<const SymbolSet>> symbolSet = std::nullopt);
};

/// Wrapper class to simplify benchmarking and the collection of statics.
class FlayServiceWrapper {
    /// The series of control plane updates which is applied after Flay service has started.
    std::vector<p4::v1::WriteRequest> controlPlaneUpdates;

 private:
    /// Helper function to retrieve a list of files matching a pattern.
    static std::vector<std::string> findFiles(const std::string &pattern);

 public:
    FlayServiceWrapper() = default;

    /// Try to parse the provided pattern into update files and convert them to control-plane
    /// updates.
    int parseControlUpdatesFromPattern(const std::string &pattern);

    /// Compute some statistics on the changes in the program and print them out.
    static void recordProgramChange(const FlayService &service);

    int run(const FlayServiceOptions &serviceOptions, const IR::P4Program *originalProgram,
            const FlayCompilerResult &compilerResult, const ReachabilityMap &reachabilityMap,
            const ControlPlaneConstraints &initialControlPlaneConstraints) const;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_SERVICE_FLAY_SERVER_H_
