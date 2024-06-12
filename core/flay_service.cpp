#include "backends/p4tools/modules/flay/core/flay_service.h"

#include <glob.h>

#include <cstdlib>
#include <fstream>
#include <optional>
#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/analysis.h"
#include "backends/p4tools/modules/flay/core/z3solver_reachability.h"
#include "backends/p4tools/modules/flay/passes/specializer.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

AbstractReachabilityMap &FlayServiceBase::initializeReachabilityMap(
    ReachabilityMapType mapType, const NodeAnnotationMap &nodeAnnotationMap) {
    printInfo("Creating the reachability map...");
    AbstractReachabilityMap *initializedReachabilityMap = nullptr;
    if (mapType == ReachabilityMapType::kz3Precomputed) {
        initializedReachabilityMap = new Z3SolverReachabilityMap(nodeAnnotationMap);
    } else {
        auto *solver = new Z3Solver();
        initializedReachabilityMap = new SolverReachabilityMap(*solver, nodeAnnotationMap);
    }
    return *initializedReachabilityMap;
}

FlayServiceBase::FlayServiceBase(const FlayServiceOptions &options,
                                 const FlayCompilerResult &compilerResult,
                                 const NodeAnnotationMap &nodeAnnotationMap,
                                 ControlPlaneConstraints initialControlPlaneConstraints)
    : _options(options),
      _originalProgram(compilerResult.getOriginalProgram()),
      _midEndProgram(compilerResult.getProgram()),
      _optimizedProgram(&compilerResult.getProgram()),
      _compilerResult(compilerResult),
      _reachabilityMap(initializeReachabilityMap(options.mapType, nodeAnnotationMap)),
      _substitutionMap(*new SolverSubstitutionMap(*new Z3Solver(), nodeAnnotationMap)),
      _controlPlaneConstraints(std::move(initialControlPlaneConstraints)) {
    printInfo("Checking whether dead code can be removed with the initial configuration...");
    midEndProgram().apply(P4::ResolveReferences(&_refMap));
    if (::errorCount() > 0) {
        return;
    }
    specializeProgram();
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

ControlPlaneConstraints &FlayServiceBase::mutableControlPlaneConstraints() {
    return _controlPlaneConstraints;
}

std::optional<bool> FlayServiceBase::checkForSemanticChange(
    std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) {
    printInfo("Checking for change in reachability semantics...");
    Util::ScopedTimer timer("Check for semantics change");
    if (symbolSet.has_value() && _options.useSymbolSet) {
        return mutableReachabilityMap().recomputeReachability(symbolSet.value(),
                                                              mutableControlPlaneConstraints());
    }
    return mutableReachabilityMap().recomputeReachability(mutableControlPlaneConstraints());
}

std::pair<int, bool> FlayServiceBase::specializeProgram(
    std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) {
    Util::ScopedTimer timer("Eliminate Dead Code");

    std::optional<bool> hasChangedOpt = checkForSemanticChange(symbolSet);
    if (!hasChangedOpt.has_value()) {
        return {EXIT_FAILURE, false};
    }
    bool hasChanged = hasChangedOpt.value();
    if (!hasChanged) {
        printInfo("Received update, but semantics have not changed. No program change necessary.");
        return {EXIT_SUCCESS, hasChanged};
    }
    printInfo("Change in semantics detected.");

    auto flaySpecializer = FlaySpecializer(_refMap, _reachabilityMap, _substitutionMap);
    _optimizedProgram = midEndProgram().apply(flaySpecializer);
    // Update the list of eliminated nodes.
    _eliminatedNodes = flaySpecializer.eliminatedNodes();
    return ::errorCount() == 0 ? std::pair{EXIT_SUCCESS, hasChanged}
                               : std::pair{EXIT_FAILURE, hasChanged};
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
