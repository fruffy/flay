#include "backends/p4tools/modules/flay/core/specialization/service_wrapper.h"

#include <glob.h>

#include <cstdlib>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/options.h"

namespace P4Tools::Flay {

void FlayServiceWrapper::outputOptimizedProgram(
    const std::filesystem::path &optimizedOutputFileName) {
    auto absoluteFilePath =
        FlayOptions::get().optimizedOutputDir().value() / optimizedOutputFileName;
    _flayService.outputOptimizedProgram(absoluteFilePath);
    printInfo("Wrote optimized program to %1%", absoluteFilePath);
}

std::vector<std::string> FlayServiceWrapper::findFiles(std::string_view pattern) {
    std::vector<std::string> files;
    glob_t globResult;

    // Perform globbing.
    if (glob(pattern.data(), GLOB_TILDE, nullptr, &globResult) == 0) {
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            files.emplace_back(globResult.gl_pathv[i]);
        }
    }

    // Free allocated resources
    globfree(&globResult);

    return files;
}

std::vector<AnalysisStatistics *> FlayServiceWrapper::computeFlayServiceStatistics() const {
    return _flayService.computeFlayServiceStatistics();
}

FlayServiceWrapper::FlayServiceWrapper(const FlayCompilerResult &compilerResult,
                                       IncrementalAnalysisMap incrementalAnalysisMap)
    : _flayService(compilerResult, std::move(incrementalAnalysisMap)) {}
}  // namespace P4Tools::Flay
