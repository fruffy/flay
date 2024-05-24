#include "backends/p4tools/modules/flay/core/service_wrapper.h"

#include <glob.h>

#include <cstdlib>
#include <optional>

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

FlayServiceStatistics FlayServiceWrapper::computeFlayServiceStatistics() const {
    return _flayService.computeFlayServiceStatistics();
}

FlayServiceWrapper::FlayServiceWrapper(
    const FlayServiceOptions &serviceOptions, const FlayCompilerResult &compilerResult,
    const ReachabilityMap &reachabilityMap,
    const ControlPlaneConstraints &initialControlPlaneConstraints)
    : _flayService(serviceOptions, compilerResult, reachabilityMap,
                   initialControlPlaneConstraints) {}
}  // namespace P4Tools::Flay
