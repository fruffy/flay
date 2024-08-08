#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_PARTIAL_EVALUATOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_PARTIAL_EVALUATOR_H_

#include <cstdlib>
#include <functional>
#include <optional>

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/interpreter/compiler_result.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/core/lib/incremental_analysis.h"
#include "backends/p4tools/modules/flay/core/specialization/passes/specialization_statistics.h"
#include "backends/p4tools/modules/flay/core/specialization/reachability_map.h"
#include "backends/p4tools/modules/flay/core/specialization/substitution_map.h"
#include "backends/p4tools/modules/flay/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4Tools::Flay {

enum ReachabilityMapType { kZ3Precomputed, kDefault };

struct PartialEvaluationOptions {
    /// The type of map to initialize.
    ReachabilityMapType mapType = ReachabilityMapType::kZ3Precomputed;
};

struct PartialEvaluationStatistics : public AnalysisStatistics {
    explicit PartialEvaluationStatistics(std::vector<EliminatedReplacedPair> eliminatedNodes)
        : eliminatedNodes(std::move(eliminatedNodes)) {}

    // The nodes eliminated in the program.
    std::vector<EliminatedReplacedPair> eliminatedNodes;

    [[nodiscard]] std::string toFormattedString() const override;
    DECLARE_TYPEINFO(PartialEvaluationStatistics);
};

class PartialEvaluation : public IncrementalAnalysis {
 private:
    /// The set of active control plane constraints. These constraints are added
    /// to every solver check to compute feasibility of a program node.
    ControlPlaneConstraints _controlPlaneConstraints;

    /// The reachability map used by the server. Derived from the input argument.
    AbstractReachabilityMap *_reachabilityMap = nullptr;

    /// The expression map used by the server.
    AbstractSubstitutionMap *_substitutionMap = nullptr;

    /// A map to look up declaration references.
    P4::ReferenceMap _refMap;

    /// Options for partial evaluation.
    std::reference_wrapper<const PartialEvaluationOptions> _partialEvaluationOptions;

    /// The list of eliminated and optionally replaced nodes. Used for bookkeeping.
    std::vector<EliminatedReplacedPair> _eliminatedNodes;

    /// @returns a mutable reference reachability map.
    AbstractReachabilityMap *mutableReachabilityMap();

    /// @returns a mutable reference to the substitution map that this program
    /// info object was initialized with.
    AbstractSubstitutionMap *mutableSubstitutionMap();

    /// @returns the control plane constraints that this program info object
    /// was initialized with.
    [[nodiscard]] const ControlPlaneConstraints &controlPlaneConstraints() const;

    /// @returns a mutable reference to the control plane constraints that this
    /// program info object was initialized with.
    ControlPlaneConstraints &mutableControlPlaneConstraints();

 protected:
    std::optional<bool> checkForSemanticsChange() override;
    std::optional<bool> checkForSemanticsChange(const SymbolSet &symbolSet) override;
    std::optional<const IR::P4Program *> specializeProgram(const IR::P4Program &program) override;

 public:
    PartialEvaluation(const FlayOptions &flayOptions, const FlayCompilerResult &flayCompilerResult,
                      const ProgramInfo &programInfo,
                      const PartialEvaluationOptions &partialEvaluationOptions);

    std::optional<SymbolSet> convertControlPlaneUpdate(
        const ControlPlaneUpdate &controlPlaneUpdate) override;

    int initialize() override;

    [[nodiscard]] PartialEvaluationStatistics *computeAnalysisStatistics() const override;

    DECLARE_TYPEINFO(PartialEvaluation);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_PARTIAL_EVALUATOR_H_ */
