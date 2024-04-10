#include "backends/p4tools/modules/flay/core/flay_service.h"

#include <glob.h>

#include <cstdlib>
#include <fstream>
#include <optional>
#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/core/z3solver_reachability.h"
#include "backends/p4tools/modules/flay/options.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

AbstractReachabilityMap &FlayServiceBase::initializeReachabilityMap(
    ReachabilityMapType mapType, const ReachabilityMap &reachabilityMap) {
    printInfo("Creating the reachability map...");
    AbstractReachabilityMap *initializedReachabilityMap = nullptr;
    if (mapType == ReachabilityMapType::kz3Precomputed) {
        initializedReachabilityMap = new Z3SolverReachabilityMap(reachabilityMap);
    } else {
        auto *solver = new Z3Solver();
        initializedReachabilityMap = new SolverReachabilityMap(*solver, reachabilityMap);
    }
    return *initializedReachabilityMap;
}

FlayServiceBase::FlayServiceBase(const FlayServiceOptions &options,
                                 const FlayCompilerResult &compilerResult,
                                 const ReachabilityMap &reachabilityMap,
                                 ControlPlaneConstraints initialControlPlaneConstraints)
    : options(options),
      originalProgram(compilerResult.getOriginalProgram()),
      optimizedProgram(&originalProgram.get()),
      compilerResult(compilerResult),
      reachabilityMap(initializeReachabilityMap(options.mapType, reachabilityMap)),
      controlPlaneConstraints(std::move(initialControlPlaneConstraints)) {
    printInfo("Checking whether dead code can be removed with the initial configuration...");
    originalProgram.get().apply(P4::ResolveReferences(&refMap));
    if (::errorCount() > 0) {
        return;
    }
    elimControlPlaneDeadCode();
}

int FlayServiceBase::updateControlPlaneConstraintsWithEntityMessage(
    const p4::v1::Entity &entity, const ::p4::v1::Update_Type &updateType, SymbolSet &symbolSet) {
    return ProtobufDeserializer::updateControlPlaneConstraintsWithEntityMessage(
        entity, *getCompilerResult().getP4RuntimeApi().p4Info, controlPlaneConstraints, updateType,
        symbolSet);
}

void FlayServiceBase::printoptimizedProgram() {
    P4::ToP4 toP4;
    optimizedProgram->apply(toP4);
}

void FlayServiceBase::outputOptimizedProgram(const std::filesystem::path &optimizedOutputFile) {
    std::ofstream output(optimizedOutputFile);
    if (!output.is_open()) {
        ::error("Could not open file %1% for writing.", optimizedOutputFile.c_str());
        return;
    }
    P4::ToP4 toP4(&output, false);
    optimizedProgram->apply(toP4);
    output.close();
}

const IR::P4Program &FlayServiceBase::getOptimizedProgram() const { return *optimizedProgram; }

const IR::P4Program &FlayServiceBase::getOriginalProgram() const { return originalProgram; }

const FlayCompilerResult &FlayServiceBase::getCompilerResult() const {
    return compilerResult.get();
}

const std::vector<EliminatedReplacedPair> &FlayServiceBase::getEliminatedNodes() const {
    return eliminatedNodes;
}

std::optional<bool> FlayServiceBase::checkForSemanticChange(
    std::optional<std::reference_wrapper<const SymbolSet>> symbolSet) const {
    printInfo("Checking for change in reachability semantics...");
    Util::ScopedTimer timer("Check for semantics change");
    if (symbolSet.has_value() && options.useSymbolSet) {
        return reachabilityMap.get().recomputeReachability(symbolSet.value(),
                                                           controlPlaneConstraints);
    }
    return reachabilityMap.get().recomputeReachability(controlPlaneConstraints);
}

std::pair<int, bool> FlayServiceBase::elimControlPlaneDeadCode(
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

    auto elimDeadCode = ElimDeadCode(refMap, reachabilityMap);
    optimizedProgram = getOriginalProgram().apply(elimDeadCode);
    // Update the list of eliminated nodes.
    eliminatedNodes = elimDeadCode.getEliminatedNodes();
    return ::errorCount() == 0 ? std::pair{EXIT_SUCCESS, hasChanged}
                               : std::pair{EXIT_FAILURE, hasChanged};
}

void FlayServiceWrapper::outputOptimizedProgram(std::filesystem::path optimizedOutputFileName) {
    auto absoluteFilePath = FlayOptions::get().getOptimizedOutputDir().value() / optimizedOutputFileName;
    flayService.outputOptimizedProgram(absoluteFilePath);
    printInfo("Outputed optimized program to %1%", absoluteFilePath);
}

std::vector<std::string> FlayServiceWrapper::findFiles(const std::string &pattern) {
    std::vector<std::string> files;
    glob_t globResult;

    // Perform globbing.
    if (glob(pattern.c_str(), GLOB_TILDE, nullptr, &globResult) == 0) {
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            files.emplace_back(globResult.gl_pathv[i]);
        }
    }

    // Free allocated resources
    globfree(&globResult);

    return files;
}

int FlayServiceWrapper::parseControlUpdatesFromPattern(const std::string &pattern) {
    auto files = findFiles(pattern);
    for (const auto &file : files) {
        auto entityOpt =
            ProtobufDeserializer::deserializeProtoObjectFromFile<p4::v1::WriteRequest>(file);
        if (!entityOpt.has_value()) {
            return EXIT_FAILURE;
        }
        controlPlaneUpdates.emplace_back(entityOpt.value());
    }
    return EXIT_SUCCESS;
}

void FlayServiceWrapper::recordProgramChange(const FlayServiceBase &service) {
    auto statementCountBefore = countStatements(service.originalProgram);
    auto statementCountAfter = countStatements(*service.optimizedProgram);
    float stmtPct = 100.0F * (1.0F - static_cast<float>(statementCountAfter) /
                                         static_cast<float>(statementCountBefore));
    printInfo("Number of statements - Before: %1% After: %2% Total reduction in statements = %3%%%",
              statementCountBefore, statementCountAfter, stmtPct);
}

int FlayServiceWrapper::run() {
    if (::errorCount() > 0) {
        ::error("Encountered errors trying to starting the service.");
        return EXIT_FAILURE;
    }
    recordProgramChange(flayService);

    /// Keeps track of how often the semantics have changed after an update.
    uint64_t semanticsChangeCounter = 0;
    if (!controlPlaneUpdates.empty()) {
        printInfo("Processing control plane updates...");
    }
    for (size_t updateIdx=0; updateIdx < controlPlaneUpdates.size(); updateIdx++) {
        const auto &controlPlaneUpdate = controlPlaneUpdates[updateIdx];
        SymbolSet symbolSet;
        for (const auto &update : controlPlaneUpdate.updates()) {
            Util::ScopedTimer timer("processWrapperMessage");
            if (flayService.updateControlPlaneConstraintsWithEntityMessage(
                    update.entity(), update.type(), symbolSet) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
        }
        auto result = flayService.elimControlPlaneDeadCode(symbolSet);
        if (result.first != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        if (result.second) {
            recordProgramChange(flayService);
            semanticsChangeCounter++;
        }
        if (FlayOptions::get().getOptimizedOutputDir() != std::nullopt) {
            std::filesystem::path fileName = "optimized." + std::to_string(updateIdx) + ".p4";
            outputOptimizedProgram(fileName);
        }
    }

    return EXIT_SUCCESS;
}

FlayServiceStatistics FlayServiceWrapper::getFlayServiceStatistics() const {
    return FlayServiceStatistics{&flayService.getOptimizedProgram(),
                                 flayService.getEliminatedNodes()};
}

}  // namespace P4Tools::Flay
