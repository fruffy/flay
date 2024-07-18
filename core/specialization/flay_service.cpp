#include "backends/p4tools/modules/flay/core/specialization/flay_service.h"

#include <glob.h>

#include <cstdlib>
#include <fstream>
#include <optional>
#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/lib/analysis.h"
#include "backends/p4tools/modules/flay/core/specialization/passes/specializer.h"
#include "backends/p4tools/modules/flay/core/specialization/z3/reachability_map.h"
#include "backends/p4tools/modules/flay/core/specialization/z3/substitution_map.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

namespace {

AbstractReachabilityMap &initializeReachabilityMap(ReachabilityMapType mapType, Z3Solver *solver,
                                                   const NodeAnnotationMap &nodeAnnotationMap) {
    printInfo("Creating the reachability map...");
    AbstractReachabilityMap *initializedReachabilityMap = nullptr;
    if (mapType == ReachabilityMapType::kZ3Precomputed) {
        initializedReachabilityMap = new Z3SolverReachabilityMap(*solver, nodeAnnotationMap);
    } else {
        initializedReachabilityMap = new IRReachabilityMap(nodeAnnotationMap);
    }
    return *initializedReachabilityMap;
}

AbstractSubstitutionMap &initializeSubstitutionMap(ReachabilityMapType mapType, Z3Solver *solver,
                                                   const NodeAnnotationMap &nodeAnnotationMap) {
    printInfo("Creating the reachability map...");
    AbstractSubstitutionMap *initializedSubstitutionMap = nullptr;
    if (mapType == ReachabilityMapType::kZ3Precomputed) {
        initializedSubstitutionMap = new Z3SolverSubstitutionMap(*solver, nodeAnnotationMap);
    } else {
        initializedSubstitutionMap = new SubstitutionMap(nodeAnnotationMap);
    }
    return *initializedSubstitutionMap;
}

}  // namespace

FlayServiceBase::FlayServiceBase(const FlayServiceOptions &options,
                                 const FlayCompilerResult &compilerResult,
                                 const NodeAnnotationMap &nodeAnnotationMap,
                                 ControlPlaneConstraints initialControlPlaneConstraints)
    : _options(options),
      _originalProgram(compilerResult.getOriginalProgram()),
      _midEndProgram(compilerResult.getProgram()),
      _optimizedProgram(&compilerResult.getOriginalProgram()),
      _compilerResult(compilerResult),
      _z3Solver(new Z3Solver()),
      _reachabilityMap(
          initializeReachabilityMap(options.mapType, _z3Solver.get(), nodeAnnotationMap)),
      _substitutionMap(
          initializeSubstitutionMap(options.mapType, _z3Solver.get(), nodeAnnotationMap)),
      _controlPlaneConstraints(std::move(initialControlPlaneConstraints)) {
    printInfo("Checking whether dead code can be removed with the initial configuration...");
    midEndProgram().apply(P4::ResolveReferences(&_refMap));
    if (::errorCount() > 0) {
        return;
    }
    checkForChangeAndSpecializeProgram();
}

void FlayServiceBase::printOptimizedProgram() const {
    P4::ToP4 toP4;
    optimizedProgram().apply(toP4);
}

void FlayServiceBase::outputOptimizedProgram(
    const std::filesystem::path &optimizedOutputFile) const {
    std::ofstream output(optimizedOutputFile);
    if (!output.is_open()) {
        ::error("Could not open file %1% for writing.", optimizedOutputFile.c_str());
        return;
    }
    P4::ToP4 toP4(&output, false);
    optimizedProgram().apply(toP4);
    output.close();
}

const IR::P4Program &FlayServiceBase::originalProgram() const { return _originalProgram; }

const IR::P4Program &FlayServiceBase::optimizedProgram() const { return *_optimizedProgram; }

const IR::P4Program &FlayServiceBase::midEndProgram() const { return _midEndProgram; }

const FlayCompilerResult &FlayServiceBase::compilerResult() const { return _compilerResult; }

const std::vector<EliminatedReplacedPair> &FlayServiceBase::eliminatedNodes() const {
    return _eliminatedNodes;
}

AbstractReachabilityMap &FlayServiceBase::mutableReachabilityMap() { return _reachabilityMap; }

AbstractSubstitutionMap &FlayServiceBase::mutableSubstitutionMap() { return _substitutionMap; }

ControlPlaneConstraints &FlayServiceBase::mutableControlPlaneConstraints() {
    return _controlPlaneConstraints;
}

const ControlPlaneConstraints &FlayServiceBase::controlPlaneConstraints() const {
    return _controlPlaneConstraints;
}

std::optional<bool> FlayServiceBase::checkForSemanticChange(const SymbolSet &symbolSet) {
    printInfo("Checking for change in program semantics...");
    Util::ScopedTimer timer("Check for semantics change with symbol set");

    auto reachabilityResult =
        mutableReachabilityMap().recomputeReachability(symbolSet, controlPlaneConstraints());
    if (!reachabilityResult.has_value()) {
        return std::nullopt;
    }
    auto substitutionResult =
        mutableSubstitutionMap().recomputeSubstitution(symbolSet, controlPlaneConstraints());
    if (!substitutionResult.has_value()) {
        return std::nullopt;
    }
    return reachabilityResult.value() || substitutionResult.value();
}

std::optional<bool> FlayServiceBase::checkForSemanticChange() {
    printInfo("Checking for change in program semantics with symbol set...");
    Util::ScopedTimer timer("Check for semantics change");
    auto reachabilityResult =
        mutableReachabilityMap().recomputeReachability(controlPlaneConstraints());
    if (!reachabilityResult.has_value()) {
        return std::nullopt;
    }
    auto substitutionResult =
        mutableSubstitutionMap().recomputeSubstitution(controlPlaneConstraints());
    if (!substitutionResult.has_value()) {
        return std::nullopt;
    }
    return reachabilityResult.value() || substitutionResult.value();
}

int FlayServiceBase::specializeProgram() {
    printInfo("Change in semantics detected.");
    auto flaySpecializer = FlaySpecializer(_refMap, _reachabilityMap, _substitutionMap);
    _optimizedProgram = originalProgram().apply(flaySpecializer);
    // Update the list of eliminated nodes.
    _eliminatedNodes = flaySpecializer.eliminatedNodes();
    return static_cast<int>(::errorCount() > 0);
}

std::pair<int, bool> FlayServiceBase::checkForChangeAndSpecializeProgram(
    const SymbolSet &symbolSet) {
    Util::ScopedTimer timer("Specialize Program with SymbolSet");

    std::optional<bool> hasChangedOpt = checkForSemanticChange(symbolSet);
    if (!hasChangedOpt.has_value()) {
        return {EXIT_FAILURE, false};
    }
    bool hasChanged = hasChangedOpt.value();
    if (!hasChanged) {
        printInfo("Received update, but semantics have not changed. No program change necessary.");
        return {EXIT_SUCCESS, hasChanged};
    }
    return {specializeProgram(), hasChanged};
}

std::pair<int, bool> FlayServiceBase::checkForChangeAndSpecializeProgram() {
    Util::ScopedTimer timer("Specialize Program");

    std::optional<bool> hasChangedOpt = checkForSemanticChange();
    if (!hasChangedOpt.has_value()) {
        return {EXIT_FAILURE, false};
    }
    bool hasChanged = hasChangedOpt.value();
    if (!hasChanged) {
        printInfo("Received update, but semantics have not changed. No program change necessary.");
        return {EXIT_SUCCESS, hasChanged};
    }
    return {specializeProgram(), hasChanged};
}

void FlayServiceBase::recordProgramChange() const {
    auto statementCountBefore = countStatements(midEndProgram());
    auto statementCountAfter = countStatements(optimizedProgram());
    float stmtPct = 100.0F * (1.0F - static_cast<float>(statementCountAfter) /
                                         static_cast<float>(statementCountBefore));
    printInfo("Number of statements - Before: %1% After: %2% Total reduction in statements = %3%%%",
              statementCountBefore, statementCountAfter, stmtPct);
}

FlayServiceStatistics FlayServiceBase::computeFlayServiceStatistics() const {
    auto statementCountBefore = countStatements(midEndProgram());
    auto statementCountAfter = countStatements(optimizedProgram());
    auto cyclomaticComplexity = computeCyclomaticComplexity(midEndProgram());
    auto numParsersPaths = ParserPathsCounter::computeParserPaths(midEndProgram());

    return FlayServiceStatistics{&optimizedProgram(), eliminatedNodes(),    statementCountBefore,
                                 statementCountAfter, cyclomaticComplexity, numParsersPaths};
}

}  // namespace P4Tools::Flay
