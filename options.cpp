#include "backends/p4tools/modules/flay/options.h"

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

const std::set<std::string> FlayOptions::SUPPORTED_CONFIG_EXTENSIONS = {".proto"};

FlayOptions &FlayOptions::get() {
    static FlayOptions INSTANCE;
    return INSTANCE;
}

const char *FlayOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for Flay.");
}

FlayOptions::FlayOptions()
    : AbstractP4cToolOptions("Remove control-plane dead code from a P4 program.") {
    registerOption(
        "--config-file", "controlPlaneConfig",
        [this](const char *arg) {
            controlPlaneConfig = std::filesystem::path(arg);
            if (!std::filesystem::exists(controlPlaneConfig.value())) {
                ::error("%1% does not exist. Please provide a valid file path.",
                        controlPlaneConfig.value().c_str());
                return false;
            }
            auto extension = controlPlaneConfig.value().extension().c_str();
            if (SUPPORTED_CONFIG_EXTENSIONS.find(extension) == SUPPORTED_CONFIG_EXTENSIONS.end()) {
                ::error(
                    "File %1% does not have a supported extension. Supported extensions are "
                    "%2%.",
                    controlPlaneConfig.value().c_str(),
                    Utils::containerToString(SUPPORTED_CONFIG_EXTENSIONS));
            }
            return true;
        },
        "The path to the control plane configuration file. The instructions in this file will be "
        "converted into a semantic representation and merged with the representation of the P4 "
        "file.");
}

std::filesystem::path FlayOptions::getControlPlaneConfig() const {
    return controlPlaneConfig.value();
}

bool FlayOptions::hasControlPlaneConfig() const { return controlPlaneConfig.has_value(); }

}  // namespace P4Tools
