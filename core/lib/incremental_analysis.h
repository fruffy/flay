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

struct ControlPlaneUpdate : public ICastable {
    DECLARE_TYPEINFO(ControlPlaneUpdate);
};

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
    std::reference_wrapper<const FlayOptions> _flayOptions;
    std::reference_wrapper<const FlayCompilerResult> _flayCompilerResult;
    std::reference_wrapper<const ProgramInfo> _programInfo;

 protected:
    virtual std::optional<bool> checkForSemanticsChange() = 0;
    virtual std::optional<bool> checkForSemanticsChange(const SymbolSet &symbolSet) = 0;

    virtual std::optional<SymbolSet> convertControlPlaneUpdate(
        const ControlPlaneUpdate &controlPlaneUpdate) = 0;

    [[nodiscard]] const FlayOptions &flayOptions() const { return _flayOptions; }

    [[nodiscard]] const FlayCompilerResult &flayCompilerResult() const {
        return _flayCompilerResult;
    }

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

    virtual int initialize() = 0;
    [[nodiscard]] virtual std::optional<const IR::P4Program *> specializeProgram(
        const IR::P4Program &program) = 0;

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

    std::optional<const IR::P4Program *> processControlPlaneUpdate(
        const IR::P4Program &program,
        const std::vector<const ControlPlaneUpdate *> &controlPlaneUpdates) {
        SymbolSet symbolSet;
        for (const auto &update : controlPlaneUpdates) {
            ASSIGN_OR_RETURN(SymbolSet updateSymbolSet, convertControlPlaneUpdate(*update),
                             std::nullopt);
            symbolSet.insert(updateSymbolSet.begin(), updateSymbolSet.end());
        }
        ASSIGN_OR_RETURN(bool changeNeeded, checkForSemanticsChange(symbolSet), std::nullopt);
        if (!changeNeeded) {
            return &program;
        }
        printInfo("Change in semantics detected.");

        auto newProgram = specializeProgram(program);
        return newProgram;
    }

    [[nodiscard]] virtual AnalysisStatistics *computeAnalysisStatistics() const = 0;

    DECLARE_TYPEINFO(IncrementalAnalysis);
};

using IncrementalAnalysisMap = ordered_map<std::string, std::unique_ptr<IncrementalAnalysis>>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_INCREMENTAL_ANALYSIS_H_ */
