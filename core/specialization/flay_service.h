#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_FLAY_SERVICE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_FLAY_SERVICE_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/lib/incremental_analysis.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4Tools::Flay {

class StatementCounter : public Inspector {
    uint64_t _statementCount = 0;

 public:
    bool preorder(const IR::AssignmentStatement * /*statement*/) override {
        _statementCount++;
        return false;
    }
    bool preorder(const IR::MethodCallStatement * /*statement*/) override {
        _statementCount++;
        return false;
    }

    [[nodiscard]] uint64_t getStatementCount() const { return _statementCount; }
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

struct FlayServiceStatistics : public AnalysisStatistics {
    FlayServiceStatistics(const IR::P4Program *optimizedProgram, uint64_t statementCountBefore,
                          uint64_t statementCountAfter, size_t cyclomaticComplexity,
                          size_t numParsersPaths)
        : optimizedProgram(optimizedProgram),
          statementCountBefore(statementCountBefore),
          statementCountAfter(statementCountAfter),
          cyclomaticComplexity(cyclomaticComplexity),
          numParsersPaths(numParsersPaths) {}

    // The optimized program.
    const IR::P4Program *optimizedProgram;
    // The number of statements in the original program.
    uint64_t statementCountBefore;
    // The number of statements in the optimized program.
    uint64_t statementCountAfter;
    // The cyclomatic complexity of the input program.
    size_t cyclomaticComplexity;
    // The total number of paths for parsers
    size_t numParsersPaths;

    [[nodiscard]] std::string toFormattedString() const override {
        std::stringstream output;
        output << "\nstatement_count_before:" << statementCountBefore << "\n";
        output << "statement_count_after:" << statementCountAfter << "\n";
        output << "cyclomatic_complexity:" << cyclomaticComplexity << "\n";
        output << "num_parsers_paths:" << numParsersPaths << "\n";
        return output.str();
    }

    DECLARE_TYPEINFO(FlayServiceStatistics);
};

class FlayServiceBase {
 protected:
    /// The incremental analysis.
    IncrementalAnalysisMap _incrementalAnalysisMap;

    /// The original source P4 program after the front end.
    std::reference_wrapper<const IR::P4Program> _originalProgram;

    /// The P4 program after mid end optimizations.
    std::reference_wrapper<const IR::P4Program> _midEndProgram;

    /// The P4 program after Flay optimizing. Should be initialized with the
    /// initial data plane configuration
    const IR::P4Program *_optimizedProgram = nullptr;

    int specializeProgram();

 public:
    explicit FlayServiceBase(const FlayCompilerResult &compilerResult,
                             IncrementalAnalysisMap incrementalAnalysisMap);

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

    /// Compute some statistics on the changes in the program and print them out.
    void recordProgramChange() const;

    int processControlPlaneUpdate(const ControlPlaneUpdate &controlPlaneUpdate);
    int processControlPlaneUpdate(
        const std::vector<const ControlPlaneUpdate *> &controlPlaneUpdates);

    /// Compute and return some statistics on the changes in the program.
    [[nodiscard]] std::vector<AnalysisStatistics *> computeFlayServiceStatistics() const;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_FLAY_SERVICE_H_
