#include "backends/p4tools/modules/flay/core/specialization/flay_service.h"

#include <glob.h>

#include <cstdlib>
#include <fstream>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/lib/analysis.h"
#include "frontends/p4/toP4/toP4.h"
#include "lib/error.h"
#include "lib/timer.h"

namespace P4::P4Tools::Flay {

FlayServiceBase::FlayServiceBase(const FlayCompilerResult &compilerResult,
                                 IncrementalAnalysisMap incrementalAnalysisMap)
    : _incrementalAnalysisMap(std::move(incrementalAnalysisMap)),
      _originalProgram(compilerResult.getOriginalProgram()),
      _midEndProgram(compilerResult.getProgram()),
      _optimizedProgram(&compilerResult.getOriginalProgram()) {
    printInfo("Specializing the program unconditionally...");
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
        ::P4::error("Could not open file %1% for writing.", optimizedOutputFile.c_str());
        return;
    }
    P4::ToP4 toP4(&output, false);
    optimizedProgram().apply(toP4);
    output.close();
}

const IR::P4Program &FlayServiceBase::originalProgram() const { return _originalProgram; }

const IR::P4Program &FlayServiceBase::optimizedProgram() const { return *_optimizedProgram; }

const IR::P4Program &FlayServiceBase::midEndProgram() const { return _midEndProgram; }

int FlayServiceBase::specializeProgram() {
    Util::ScopedTimer timer("Specialize program");
    const auto *optimizedProg = &originalProgram();
    for (const auto &[analysisName, incrementalAnalysis] : _incrementalAnalysisMap) {
        auto optProgram = incrementalAnalysis->specializeProgram(*optimizedProg);
        if (!optProgram.has_value()) {
            return EXIT_FAILURE;
        }
        optimizedProg = optProgram.value();
    }
    _optimizedProgram = optimizedProg;

    return EXIT_SUCCESS;
}

int FlayServiceBase::processControlPlaneUpdate(const ControlPlaneUpdate &controlPlaneUpdate) {
    Util::ScopedTimer timer("Processing control plane update");
    const auto *optimizedProg = &originalProgram();
    _updateCount++;
    bool hasRespecialized = false;
    for (const auto &[analysisName, incrementalAnalysis] : _incrementalAnalysisMap) {
        auto optProgram =
            incrementalAnalysis->processControlPlaneUpdate(*optimizedProg, controlPlaneUpdate);
        if (!optProgram.has_value()) {
            return EXIT_FAILURE;
        }
        if (optProgram.value() != nullptr) {
            _respecializationCount++;
            optimizedProg = optProgram.value();
            _optimizedProgram = optimizedProg;
            hasRespecialized = true;
        }
    }
    if (hasRespecialized) {
        _respecializationCount++;
    }
    return EXIT_SUCCESS;
}

int FlayServiceBase::processControlPlaneUpdate(
    const std::vector<const ControlPlaneUpdate *> &controlPlaneUpdates) {
    Util::ScopedTimer timer("Processing control plane updates");
    _updateCount += controlPlaneUpdates.size();
    const auto *optimizedProg = &originalProgram();
    bool hasRespecialized = false;
    for (const auto &[analysisName, incrementalAnalysis] : _incrementalAnalysisMap) {
        auto optProgram =
            incrementalAnalysis->processControlPlaneUpdate(*optimizedProg, controlPlaneUpdates);
        if (!optProgram.has_value()) {
            return EXIT_FAILURE;
        }
        if (optProgram.value() != nullptr) {
            optimizedProg = optProgram.value();
            _optimizedProgram = optimizedProg;
            hasRespecialized = true;
        }
    }
    if (hasRespecialized) {
        _respecializationCount++;
    }
    return EXIT_SUCCESS;
}

void FlayServiceBase::recordProgramChange() const {
    auto statementCountBefore = countStatements(midEndProgram());
    auto statementCountAfter = countStatements(optimizedProgram());
    float stmtPct = 100.0F * (1.0F - static_cast<float>(statementCountAfter) /
                                         static_cast<float>(statementCountBefore));
    printInfo("Number of statements - Before: %1% After: %2% Total reduction in statements = %3%%%",
              statementCountBefore, statementCountAfter, stmtPct);
}

FlayServiceStatisticsMap FlayServiceBase::computeFlayServiceStatistics() const {
    auto statementCountBefore = countStatements(midEndProgram());
    auto statementCountAfter = countStatements(optimizedProgram());
    auto cyclomaticComplexity = computeCyclomaticComplexity(midEndProgram());
    auto numParsersPaths = ParserPathsCounter::computeParserPaths(midEndProgram());
    FlayServiceStatisticsMap statistics;
    for (const auto &[analysisName, incrementalAnalysis] : _incrementalAnalysisMap) {
        statistics.emplace(analysisName, incrementalAnalysis->computeAnalysisStatistics());
    }
    statistics.emplace(
        "main", new FlayServiceStatistics(&optimizedProgram(), statementCountBefore,
                                          statementCountAfter, cyclomaticComplexity,
                                          numParsersPaths, updateCount(), respecializationCount()));
    return statistics;
}

}  // namespace P4::P4Tools::Flay
