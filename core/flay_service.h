#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_FLAY_SERVICE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_FLAY_SERVICE_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/compiler_result.h"
#include "backends/p4tools/modules/flay/core/reachability.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"
#include "frontends/p4/toP4/toP4.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4Tools::Flay {

class StatementCounter : public Inspector {
    uint64_t statementCount_ = 0;

 public:
    bool preorder(const IR::AssignmentStatement * /*statement*/) override {
        statementCount_++;
        return false;
    }
    bool preorder(const IR::MethodCallStatement * /*statement*/) override {
        statementCount_++;
        return false;
    }

    [[nodiscard]] uint64_t getStatementCount() const { return statementCount_; }
};

inline uint64_t countStatements(const IR::P4Program &prog) {
    auto *counter = new StatementCounter();
    prog.apply(*counter);
    return counter->getStatementCount();
}

inline double measureProgramSize(const IR::P4Program &prog) {
    auto *programStream = new std::stringstream;
    auto *toP4 = new P4::ToP4(programStream, false);
    prog.apply(*toP4);
    return static_cast<double>(programStream->str().length());
}

inline double measureSizeDifference(const IR::P4Program &programBefore,
                                    const IR::P4Program &programAfter) {
    double beforeLength = measureProgramSize(programBefore);

    return (beforeLength - measureProgramSize(programAfter)) * (100.0 / beforeLength);
}

enum ReachabilityMapType { kz3Precomputed, kZ3Default };

struct FlayServiceOptions {
    /// If useSymbolSet is true, we only check whether the symbols in the set have changed.
    bool useSymbolSet = true;
    /// The type of map to initialize.
    ReachabilityMapType mapType = ReachabilityMapType::kz3Precomputed;
};

class FlayServiceBase {
    friend class FlayServiceWrapper;

 protected:
    // Configuration options for the service.
    FlayServiceOptions options;

    /// The original source P4 program.
    std::reference_wrapper<const IR::P4Program> originalProgram;

    /// The P4 program after optimizing. Should be initialized with the initial data plane
    /// configuration
    const IR::P4Program *optimizedProgram = nullptr;

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

    /// The list of eliminated and optionally replaced nodes. Used for bookkeeping.
    std::vector<EliminatedReplacedPair> eliminatedNodes;

    /// Compute whether the semantics of the program under this control plane configuration have
    /// changed since the last update.
    // @returns std::nullopt if an error occurred during the computation.
    [[nodiscard]] std::optional<bool> checkForSemanticChange(
        std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) const;

    /// Update the internal  control plane constraints with the given entity message.
    /// Private method used for testing.
    int updateControlPlaneConstraintsWithEntityMessage(const p4::v1::Entity &entity,
                                                       const ::p4::v1::Update_Type &updateType,
                                                       SymbolSet &symbolSet);

 public:
    explicit FlayServiceBase(const FlayServiceOptions &options,
                             const FlayCompilerResult &compilerResult,
                             const ReachabilityMap &reachabilityMap,
                             ControlPlaneConstraints initialControlPlaneConstraints);

    /// Print the optimized program to stdout;
    void printoptimizedProgram();

    /// Output the optimized program to file.
    void outputOptimizedProgram(const std::filesystem::path &optimizedOutputFile);

    static AbstractReachabilityMap &initializeReachabilityMap(
        ReachabilityMapType mapType, const ReachabilityMap &reachabilityMap);

    /// @returns the optimized program.
    [[nodiscard]] const IR::P4Program &getOptimizedProgram() const;

    /// @returns the original program.
    [[nodiscard]] const IR::P4Program &getOriginalProgram() const;

    /// @returns the list of eliminated (and potential replaced) nodes.
    [[nodiscard]] const std::vector<EliminatedReplacedPair> &getEliminatedNodes() const;

    /// @returns a reference to the compiler result that this program info object was initialized
    /// with.
    [[nodiscard]] const FlayCompilerResult &getCompilerResult() const;

    /// Run dead code elimination on the original P4 program.
    std::pair<int, bool> elimControlPlaneDeadCode(
        std::optional<std::reference_wrapper<const SymbolSet>> symbolSet = std::nullopt);
};

struct FlayServiceStatistics {
    // The optimized program.
    const IR::P4Program *optimizedProgram;
    // The nodes eliminated in the program.
    std::vector<EliminatedReplacedPair> eliminatedNodes;
};

/// Wrapper class to simplify benchmarking and the collection of statics.
class FlayServiceWrapper {
    /// The series of control plane updates which is applied after Flay service has started.
    std::vector<std::string> controlPlaneUpdateFileNames;
    std::vector<p4::v1::WriteRequest> controlPlaneUpdates;

 private:
    /// Helper function to retrieve a list of files matching a pattern.
    static std::vector<std::string> findFiles(const std::string &pattern);

    FlayServiceBase flayService;

 public:
    FlayServiceWrapper(const FlayServiceOptions &serviceOptions,
                       const FlayCompilerResult &compilerResult,
                       const ReachabilityMap &reachabilityMap,
                       const ControlPlaneConstraints &initialControlPlaneConstraints)
        : flayService(serviceOptions, compilerResult, reachabilityMap,
                      initialControlPlaneConstraints) {}

    /// Output the optimized program to file.
    void outputOptimizedProgram(std::filesystem::path optimizedOutputFile);

    /// Try to parse the provided pattern into update files and convert them to control-plane
    /// updates.
    int parseControlUpdatesFromPattern(const std::string &pattern);

    /// Compute some statistics on the changes in the program and print them out.
    static void recordProgramChange(const FlayServiceBase &service);

    [[nodiscard]] int run();

    /// @returns the optimized program after running Flay.
    [[nodiscard]] FlayServiceStatistics getFlayServiceStatistics() const;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_FLAY_SERVICE_H_
