#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_H_

#include <vector>

#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"
#include "backends/p4tools/modules/flay/core/specialization/flay_service.h"

namespace P4Tools::Flay {

/// Wrapper class to simplify benchmarking and the collection of statistics.
class FlayServiceWrapper {
 protected:
    /// The series of control plane updates which is applied after Flay service has started.
    std::vector<std::string> _controlPlaneUpdateFileNames;

    /// Helper function to retrieve a list of files matching a pattern.
    static std::vector<std::string> findFiles(std::string_view pattern);

    /// The Flay service that is being wrapped.
    FlayServiceBase _flayService;

 public:
    FlayServiceWrapper(const FlayCompilerResult &compilerResult,
                       IncrementalAnalysisMap incrementalAnalysisMap);
    virtual ~FlayServiceWrapper() = default;

    /// Try to parse the provided pattern into update files and convert them to control-plane
    /// updates.
    virtual int parseControlUpdatesFromPattern(std::string_view pattern) = 0;

    /// Run the Flay service.
    [[nodiscard]] virtual int run() = 0;

    /// Output the optimized program to file.
    void outputOptimizedProgram(const std::filesystem::path &optimizedOutputFile);

    /// Compute and return some statistics on the changes in the program.
    [[nodiscard]] std::vector<AnalysisStatistics *> computeFlayServiceStatistics() const;
};

}  // namespace P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_H_
