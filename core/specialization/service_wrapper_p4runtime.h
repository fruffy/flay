#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_P4RUNTIME_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_P4RUNTIME_H_

#include <utility>
#include <vector>

#include "backends/p4tools/modules/flay/core/interpreter/node_map.h"
#include "backends/p4tools/modules/flay/core/specialization/service_wrapper.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::Flay {

/// Wrapper class to simplify benchmarking and the collection of statistics.
class P4RuntimeFlayServiceWrapper : public FlayServiceWrapper {
    /// The parsed series of control plane updates which is applied after Flay service has started.
    std::vector<p4::v1::WriteRequest> _controlPlaneUpdates;

 public:
    P4RuntimeFlayServiceWrapper(const FlayCompilerResult &compilerResult,
                                IncrementalAnalysisMap incrementalAnalysisMap)
        : FlayServiceWrapper(compilerResult, std::move(incrementalAnalysisMap)) {}

    /// Try to parse the provided pattern into update files and convert them to control-plane
    /// updates.
    int parseControlUpdatesFromPattern(std::string_view pattern) override;

    /// Run the Flay service.
    [[nodiscard]] int run() override;
};

}  // namespace P4::P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_P4RUNTIME_H_
