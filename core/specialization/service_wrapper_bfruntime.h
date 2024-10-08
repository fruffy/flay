#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_BFRUNTIME_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_BFRUNTIME_H_

#include <utility>
#include <vector>

#include "backends/p4tools/modules/flay/core/specialization/service_wrapper.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/common/control_plane/bfruntime/bfruntime.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::Flay {

/// Wrapper class to simplify benchmarking and the collection of statistics.
class BfRuntimeFlayServiceWrapper : public FlayServiceWrapper {
    /// The parsed series of control plane updates which is applied after Flay service has started.
    std::vector<bfrt_proto::WriteRequest> _controlPlaneUpdates;

 public:
    BfRuntimeFlayServiceWrapper(const FlayCompilerResult &compilerResult,
                                IncrementalAnalysisMap incrementalAnalysisMap)
        : FlayServiceWrapper(compilerResult, std::move(incrementalAnalysisMap)) {}

    /// Try to parse the provided pattern into update files and convert them to control-plane
    /// updates.
    int parseControlUpdatesFromPattern(std::string_view pattern) override;

    /// Run the Flay service.
    [[nodiscard]] int run() override;
};

}  // namespace P4::P4Tools::Flay

#endif  // BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_SERVICE_WRAPPER_BFRUNTIME_H_
