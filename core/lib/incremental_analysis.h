#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_INCREMENTAL_ANALYSIS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_INCREMENTAL_ANALYSIS_H_

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/control_plane/symbols.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/core/lib/return_macros.h"
#include "backends/p4tools/modules/flay/options.h"
#include "lib/castable.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/common/control_plane/bfruntime/bfruntime.pb.h"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4Tools::Flay {

/// A thin wrapper for control-plane updates.
struct ControlPlaneUpdate : public ICastable {
    DECLARE_TYPEINFO(ControlPlaneUpdate);
};

/// Statistics of the analysis for bookkeeping.
struct AnalysisStatistics : public ICastable {
    /// Convert the statistics into a formatted string that can be written to a file.
    [[nodiscard]] virtual std::string toFormattedString() const = 0;

    friend std::ostream &operator<<(std::ostream &stream, const AnalysisStatistics &statistics) {
        stream << statistics.toFormattedString();
        return stream;
    }

    DECLARE_TYPEINFO(AnalysisStatistics);
};

struct P4RuntimeControlPlaneUpdate : ControlPlaneUpdate {
    explicit P4RuntimeControlPlaneUpdate(const p4::v1::Update &update) : update(update) {}
    const p4::v1::Update &update;
    DECLARE_TYPEINFO(P4RuntimeControlPlaneUpdate);
};

struct BfRuntimeControlPlaneUpdate : ControlPlaneUpdate {
    explicit BfRuntimeControlPlaneUpdate(const bfrt_proto::Update &update) : update(update) {}
    const bfrt_proto::Update &update;
    DECLARE_TYPEINFO(BfRuntimeControlPlaneUpdate);
};

class IncrementalAnalysis : public ICastable {
 private:
    /// The options passed to Flay.
    std::reference_wrapper<const FlayOptions> _flayOptions;
    /// The result of the Flay compiler.
    std::reference_wrapper<const FlayCompilerResult> _flayCompilerResult;
    /// The program info derived from the Flay compiler.
    std::reference_wrapper<const ProgramInfo> _programInfo;

 protected:
    /// Check whether the semantics of the program have changed.
    /// Returns true if yes, std::nullopt if an error has occurred.
    virtual std::optional<bool> checkForSemanticsChange() = 0;

    /// Check whether the semantics of the program have changed.
    /// Only perform this check for the symbols in the given set.
    /// Returns true if yes, std::nullopt if an error has occurred.
    virtual std::optional<bool> checkForSemanticsChange(const SymbolSet &symbolSet) = 0;

    /// Convert an incoming control plane update and extract the affected symbols.
    /// These symbols can be mapped to constructs in the data plane program.
    /// Returns std::nullopt if an error has occurred.
    virtual std::optional<SymbolSet> convertControlPlaneUpdate(
        const ControlPlaneUpdate &controlPlaneUpdate) = 0;

    /// Get the options passed to Flay.
    [[nodiscard]] const FlayOptions &flayOptions() const { return _flayOptions; }

    /// Get the result of the Flay compiler.
    [[nodiscard]] const FlayCompilerResult &flayCompilerResult() const {
        return _flayCompilerResult;
    }

    /// Get the program info derived from the Flay compiler.
    [[nodiscard]] const ProgramInfo &programInfo() const { return _programInfo; }

 public:
    IncrementalAnalysis(const FlayOptions &flayOptions,
                        const FlayCompilerResult &flayCompilerResult,
                        const ProgramInfo &programInfo)
        : _flayOptions(flayOptions),
          _flayCompilerResult(flayCompilerResult),
          _programInfo(programInfo) {}
    IncrementalAnalysis(const IncrementalAnalysis &) = delete;
    IncrementalAnalysis(IncrementalAnalysis &&) = delete;
    IncrementalAnalysis &operator=(const IncrementalAnalysis &) = delete;
    IncrementalAnalysis &operator=(IncrementalAnalysis &&) = delete;
    ~IncrementalAnalysis() override = default;

    /// Initialize the incremental analysis and prepare for incremental specialization.
    /// Collect data from the input program which is used for incremental specialization.
    /// Perform an initial specialization pass.
    virtual int initialize() = 0;

    /// Specialize the program unconditionally without checking whether semantics have changed.
    /// Can be used in the initialization phase.
    [[nodiscard]] virtual std::optional<const IR::P4Program *> specializeProgram(
        const IR::P4Program &program) = 0;

    /// Receive a single control-plane update, convert the update to its intermediate
    /// representation needed for the respective incremental analysis, check whether the update
    /// affect the semantics of the program, and specialize the program if necessary.
    std::optional<const IR::P4Program *> processControlPlaneUpdate(
        const IR::P4Program &program, const ControlPlaneUpdate &controlPlaneUpdate) {
        ASSIGN_OR_RETURN(SymbolSet symbolSet, convertControlPlaneUpdate(controlPlaneUpdate),
                         std::nullopt);
        ASSIGN_OR_RETURN(bool changeNeeded, checkForSemanticsChange(symbolSet), std::nullopt);
        if (!changeNeeded) {
            return &program;
        }
        printInfo("Change in semantics detected.");

        auto newProgram = specializeProgram(program);
        return newProgram;
    }

    /// Receive a series of control plane updates, convert each update to its intermediate
    /// representation needed for the respective incremental analysis, check whether the updates
    /// affect the semantics of the program, and specialize the program if necessary.
    std::optional<const IR::P4Program *> processControlPlaneUpdate(
        const IR::P4Program &program,
        const std::vector<const ControlPlaneUpdate *> &controlPlaneUpdates) {
        SymbolSet symbolSet;
        for (const auto &update : controlPlaneUpdates) {
            ASSIGN_OR_RETURN(SymbolSet updateSymbolSet, convertControlPlaneUpdate(*update),
                             std::nullopt);
            symbolSet.insert(updateSymbolSet.begin(), updateSymbolSet.end());
        }
        if (flayOptions().useSymbolSet()) {
            ASSIGN_OR_RETURN(bool changeNeeded, checkForSemanticsChange(symbolSet), std::nullopt);
            if (!changeNeeded) {
                return &program;
            }
            printInfo("Change in semantics detected.");
        }

        return specializeProgram(program);
    }

    /// Return statistics of the analysis for bookkeeping.
    [[nodiscard]] virtual AnalysisStatistics *computeAnalysisStatistics() const = 0;

    DECLARE_TYPEINFO(IncrementalAnalysis);
};

/// A map of incremental analysis instances. The name can be used for disambiguation and filtering.
using IncrementalAnalysisMap = ordered_map<std::string, std::unique_ptr<IncrementalAnalysis>>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_INCREMENTAL_ANALYSIS_H_ */
