#include "backends/p4tools/modules/flay/core/interpreter/partial_evaluator.h"

#include <cstdlib>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/control_plane/bfruntime/protobuf.h"
#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/control_plane/p4runtime/protobuf.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/core/interpreter/target.h"
#include "backends/p4tools/modules/flay/core/lib/incremental_analysis.h"
#include "backends/p4tools/modules/flay/core/lib/return_macros.h"
#include "backends/p4tools/modules/flay/core/specialization/passes/specializer.h"
#include "backends/p4tools/modules/flay/core/specialization/z3/reachability_map.h"
#include "backends/p4tools/modules/flay/core/specialization/z3/substitution_map.h"
#include "backends/p4tools/modules/flay/options.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

namespace {

AbstractReachabilityMap *initializeReachabilityMap(ReachabilityMapType mapType,
                                                   const NodeAnnotationMap &nodeAnnotationMap) {
    printInfo("Creating the reachability map...");
    AbstractReachabilityMap *initializedReachabilityMap = nullptr;
    if (mapType == ReachabilityMapType::kZ3Precomputed) {
        initializedReachabilityMap = new Z3SolverReachabilityMap(nodeAnnotationMap);
    } else {
        initializedReachabilityMap = new IRReachabilityMap(nodeAnnotationMap);
    }
    return initializedReachabilityMap;
}

AbstractSubstitutionMap *initializeSubstitutionMap(ReachabilityMapType mapType,
                                                   const NodeAnnotationMap &nodeAnnotationMap) {
    printInfo("Creating the substitution map...");
    AbstractSubstitutionMap *initializedSubstitutionMap = nullptr;
    if (mapType == ReachabilityMapType::kZ3Precomputed) {
        initializedSubstitutionMap = new Z3SolverSubstitutionMap(nodeAnnotationMap);
    } else {
        initializedSubstitutionMap = new IrSubstitutionMap(nodeAnnotationMap);
    }
    return initializedSubstitutionMap;
}

}  // namespace

AbstractReachabilityMap *PartialEvaluation::mutableReachabilityMap() { return _reachabilityMap; }

AbstractSubstitutionMap *PartialEvaluation::mutableSubstitutionMap() { return _substitutionMap; }

const ControlPlaneConstraints &PartialEvaluation::controlPlaneConstraints() const {
    return _controlPlaneConstraints;
}

ControlPlaneConstraints &PartialEvaluation::mutableControlPlaneConstraints() {
    return _controlPlaneConstraints;
}

std::optional<bool> PartialEvaluation::checkForSemanticsChange() {
    printInfo("Checking for change in program semantics...");
    Util::ScopedTimer timer("Check for semantics change");

    auto reachabilityResult =
        mutableReachabilityMap()->recomputeReachability(controlPlaneConstraints());
    if (!reachabilityResult.has_value()) {
        return std::nullopt;
    }
    auto substitutionResult =
        mutableSubstitutionMap()->recomputeSubstitution(controlPlaneConstraints());
    if (!substitutionResult.has_value()) {
        return std::nullopt;
    }
    return reachabilityResult.value() || substitutionResult.value();
}

std::optional<bool> PartialEvaluation::checkForSemanticsChange(const SymbolSet &symbolSet) {
    printInfo("Checking for change in program semantics with symbol set...");
    Util::ScopedTimer timer("Check for semantics change with symbol set");

    auto reachabilityResult =
        mutableReachabilityMap()->recomputeReachability(symbolSet, controlPlaneConstraints());
    if (!reachabilityResult.has_value()) {
        return std::nullopt;
    }
    auto substitutionResult =
        mutableSubstitutionMap()->recomputeSubstitution(symbolSet, controlPlaneConstraints());
    if (!substitutionResult.has_value()) {
        return std::nullopt;
    }
    return reachabilityResult.value() || substitutionResult.value();
}

std::optional<const IR::P4Program *> PartialEvaluation::specializeProgram(
    const IR::P4Program &program) {
    auto flaySpecializer = FlaySpecializer(_refMap, *_reachabilityMap, *_substitutionMap);
    const auto *optimizedProgram = program.apply(flaySpecializer);
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    // Update the list of eliminated nodes.
    _eliminatedNodes = flaySpecializer.eliminatedNodes();
    return optimizedProgram;
}

std::optional<SymbolSet> PartialEvaluation::convertControlPlaneUpdate(
    const ControlPlaneUpdate &controlPlaneUpdate) {
    SymbolSet symbolSet;
    if (const auto *p4RuntimeUpdate = controlPlaneUpdate.to<P4RuntimeControlPlaneUpdate>()) {
        auto result = P4Runtime::updateControlPlaneConstraintsWithEntityMessage(
            p4RuntimeUpdate->update.entity(), *flayCompilerResult().getP4RuntimeApi().p4Info,
            _controlPlaneConstraints, p4RuntimeUpdate->update.type(), symbolSet);
        if (result != EXIT_SUCCESS) {
            return std::nullopt;
        }
    } else if (const auto *bfRuntimeUpdate = controlPlaneUpdate.to<BfRuntimeControlPlaneUpdate>()) {
        auto result = BfRuntime::updateControlPlaneConstraintsWithEntityMessage(
            bfRuntimeUpdate->update.entity(), *flayCompilerResult().getP4RuntimeApi().p4Info,
            _controlPlaneConstraints, bfRuntimeUpdate->update.type(), symbolSet);
        if (result != EXIT_SUCCESS) {
            return std::nullopt;
        }
    } else {
        ::error("Unknown control plane update type: %1%", typeid(controlPlaneUpdate).name());
        return std::nullopt;
    }
    return symbolSet;
}

PartialEvaluation::PartialEvaluation(const FlayOptions &flayOptions,
                                     const FlayCompilerResult &flayCompilerResult,
                                     const ProgramInfo &programInfo,
                                     const PartialEvaluationOptions &partialEvaluationOptions)
    : IncrementalAnalysis(flayOptions, flayCompilerResult, programInfo),
      _partialEvaluationOptions(partialEvaluationOptions) {
    flayCompilerResult.getProgram().apply(P4::ResolveReferences(&_refMap));
}

int PartialEvaluation::initialize() {
    printInfo("Computing initial control plane constraints...");
    // Gather the initial control-plane configuration. Also from a file input,
    // if present.
    ASSIGN_OR_RETURN(
        _controlPlaneConstraints,
        FlayTarget::computeControlPlaneConstraints(flayCompilerResult(), flayOptions()),
        EXIT_FAILURE);

    ExecutionState executionState(&programInfo().getP4Program());

    printInfo("Starting data plane analysis...");
    Util::ScopedTimer timer("Data plane analysis");
    const auto *pipelineSequence = programInfo().getPipelineSequence();
    auto &stepper =
        FlayTarget::getStepper(programInfo(), mutableControlPlaneConstraints(), executionState);
    stepper.initializeState();
    for (const auto *node : *pipelineSequence) {
        node->apply(stepper);
    }
    /// Substitute any placeholder variables encountered in the execution state.
    printInfo("Substituting placeholder variables...");
    executionState.substitutePlaceholders();

    printInfo("Setting up analysis maps...");
    _reachabilityMap = initializeReachabilityMap(_partialEvaluationOptions.get().mapType,
                                                 executionState.nodeAnnotationMap());
    _substitutionMap = initializeSubstitutionMap(_partialEvaluationOptions.get().mapType,
                                                 executionState.nodeAnnotationMap());

    printInfo("Precomputing reachability and substitution maps with initial constraints...");
    auto reachabilityResult = _reachabilityMap->recomputeReachability(controlPlaneConstraints());
    if (!reachabilityResult.has_value()) {
        return EXIT_FAILURE;
    }
    auto substitutionResult =
        mutableSubstitutionMap()->recomputeSubstitution(controlPlaneConstraints());
    if (!substitutionResult.has_value()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

[[nodiscard]] PartialEvaluationStatistics *PartialEvaluation::computeAnalysisStatistics() const {
    return new PartialEvaluationStatistics{_eliminatedNodes};
}

}  // namespace P4Tools::Flay
