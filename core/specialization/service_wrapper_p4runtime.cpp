#include "backends/p4tools/modules/flay/core/specialization/service_wrapper_p4runtime.h"

#include <glob.h>

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/control_plane/protobuf_utils.h"
#include "backends/p4tools/modules/flay/options.h"
#include "lib/error.h"

namespace P4::P4Tools::Flay {

int P4RuntimeFlayServiceWrapper::parseControlUpdatesFromPattern(std::string_view pattern) {
    auto files = findFiles(pattern);
    for (const auto &file : files) {
        printInfo("Processing control plane update: %1%", file);
        auto entityOpt = Protobuf::deserializeObjectFromFile<p4::v1::WriteRequest>(file);
        if (!entityOpt.has_value()) {
            return EXIT_FAILURE;
        }
        _controlPlaneUpdateFileNames.emplace_back(std::filesystem::path(file).filename());
        _controlPlaneUpdates.emplace_back(entityOpt.value());
    }
    return EXIT_SUCCESS;
}

int P4RuntimeFlayServiceWrapper::run() {
    if (errorCount() > 0) {
        error("Encountered errors trying to starting the service.");
        return EXIT_FAILURE;
    }
    _flayService.recordProgramChange();
    if (FlayOptions::get().optimizedOutputDir() != std::nullopt) {
        outputOptimizedProgram("before_updates.p4");
    }

    /// Keeps track of how often the semantics have changed after an update.
    if (!_controlPlaneUpdates.empty()) {
        printInfo("Processing control plane updates...");
    }
    for (size_t updateIdx = 0; updateIdx < _controlPlaneUpdates.size(); updateIdx++) {
        const auto &controlPlaneUpdate = _controlPlaneUpdates[updateIdx];
        std::vector<const ControlPlaneUpdate *> p4RuntimeUpdates;
        for (const auto &update : controlPlaneUpdate.updates()) {
            p4RuntimeUpdates.emplace_back(new P4RuntimeControlPlaneUpdate(update));
        }
        auto result = _flayService.processControlPlaneUpdate(p4RuntimeUpdates);
        if (result != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }

        _flayService.recordProgramChange();
        if (FlayOptions::get().optimizedOutputDir() != std::nullopt) {
            outputOptimizedProgram(std::filesystem::path(_controlPlaneUpdateFileNames[updateIdx])
                                       .replace_extension(".p4"));
        }
    }
    return EXIT_SUCCESS;
}

}  // namespace P4::P4Tools::Flay
