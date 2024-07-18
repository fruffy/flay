#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_FLAY_SERVICE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_FLAY_SERVICE_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/interpreter/compiler_result.h"
#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"
#include "backends/p4tools/modules/flay/core/specialization/passes/specializer.h"
#include "frontends/p4/toP4/toP4.h"

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

enum ReachabilityMapType { kZ3Precomputed, kDefault };

struct FlayServiceOptions {
    /// If useSymbolSet is true, we only check whether the symbols in the set have
    /// changed.
    bool useSymbolSet = true;
    /// The type of map to initialize.
    ReachabilityMapType mapType = ReachabilityMapType::kZ3Precomputed;
};

struct FlayServiceStatistics {
    // The optimized program.
    const IR::P4Program *optimizedProgram;
    // The nodes eliminated in the program.
    std::vector<EliminatedReplacedPair> eliminatedNodes;
    // The number of statements in the original program.
    uint64_t statementCountBefore;
    // The number of statements in the optimized program.
    uint64_t statementCountAfter;
    // The cyclomatic complexity of the input program.
    size_t cyclomaticComplexity;
    // The total number of paths for parsers
    size_t numParsersPaths;
};

class FlayServiceBase {
 protected:
    // Configuration options for the service.
    FlayServiceOptions _options;

    /// The original source P4 program after the front end.
    std::reference_wrapper<const IR::P4Program> _originalProgram;

    /// The P4 program after mid end optimizations.
    std::reference_wrapper<const IR::P4Program> _midEndProgram;

    /// The P4 program after Flay optimizing. Should be initialized with the
    /// initial data plane configuration
    const IR::P4Program *_optimizedProgram = nullptr;

    /// The program info object stores the results of the compilation, which
    /// includes the P4 program and any information extracted from the program
    /// using static analysis.
    std::reference_wrapper<const FlayCompilerResult> _compilerResult;

    /// The optional Z3Solver. Only not NULL when options.mapType is set to kZ3Precomputed.
    std::unique_ptr<Z3Solver> _z3Solver;

    /// The reachability map used by the server. Derived from the input argument.
    std::reference_wrapper<AbstractReachabilityMap> _reachabilityMap;

    /// The expression map used by the server.
    std::reference_wrapper<AbstractSubstitutionMap> _substitutionMap;

    /// The set of active control plane constraints. These constraints are added
    /// to every solver check to compute feasibility of a program node.
    ControlPlaneConstraints _controlPlaneConstraints;

    /// A map to look up declaration references.
    P4::ReferenceMap _refMap;

    /// The list of eliminated and optionally replaced nodes. Used for
    /// bookkeeping.
    std::vector<EliminatedReplacedPair> _eliminatedNodes;

    /// Compute whether the semantics of the program under this control plane
    /// configuration have changed since the last update.
    // @returns std::nullopt if an error occurred during the computation.
    [[nodiscard]] std::optional<bool> checkForSemanticChange();
    [[nodiscard]] std::optional<bool> checkForSemanticChange(const SymbolSet &symbolSet);

    int specializeProgram();

 public:
    explicit FlayServiceBase(const FlayServiceOptions &options,
                             const FlayCompilerResult &compilerResult,
                             const NodeAnnotationMap &nodeAnnotationMap,
                             ControlPlaneConstraints initialControlPlaneConstraints);

    /// Print the optimized program to stdout;
    void printOptimizedProgram() const;

    /// Output the optimized program to file.
    void outputOptimizedProgram(const std::filesystem::path &optimizedOutputFile) const;

    /// @returns the original program.
    [[nodiscard]] const IR::P4Program &originalProgram() const;

    /// @returns the mid-end program.
    [[nodiscard]] const IR::P4Program &midEndProgram() const;

    /// @returns the optimized program.
    [[nodiscard]] const IR::P4Program &optimizedProgram() const;

    /// @returns the list of eliminated (and potential replaced) nodes.
    [[nodiscard]] const std::vector<EliminatedReplacedPair> &eliminatedNodes() const;

    /// @returns a reference to the compiler result that this program info object
    /// was initialized with.
    [[nodiscard]] const FlayCompilerResult &compilerResult() const;

    /// @returns a mutable reference reachability map.
    [[nodiscard]] AbstractReachabilityMap &mutableReachabilityMap();

    /// @returns a mutable reference to the substitution map that this program
    /// info object was initialized with.
    [[nodiscard]] AbstractSubstitutionMap &mutableSubstitutionMap();

    /// @returns a mutable reference to the control plane constraints that this
    /// program info object was initialized with.
    [[nodiscard]] ControlPlaneConstraints &mutableControlPlaneConstraints();

    /// @returns the control plane constraints that this program info object
    /// was initialized with.
    [[nodiscard]] const ControlPlaneConstraints &controlPlaneConstraints() const;

    /// Run specialization on the original P4 program.
    std::pair<int, bool> checkForChangeAndSpecializeProgram();
    std::pair<int, bool> checkForChangeAndSpecializeProgram(const SymbolSet &symbolSet);

    /// Compute some statistics on the changes in the program and print them out.
    void recordProgramChange() const;

    /// Compute and return some statistics on the changes in the program.
    [[nodiscard]] FlayServiceStatistics computeFlayServiceStatistics() const;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_FLAY_SERVICE_H_
