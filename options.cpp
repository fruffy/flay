#include "backends/p4tools/modules/flay/options.h"

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/options.h"
#include "backends/p4tools/modules/flay/toolname.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

FlayOptions &FlayOptions::get() {
    static FlayOptions INSTANCE;
    return INSTANCE;
}

const char *FlayOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for Flay.");
}

FlayOptions::FlayOptions(const std::string &message)
    : AbstractP4cToolOptions(Flay::TOOL_NAME, message) {
    registerOption(
        "--config-file", "controlPlaneConfig",
        [this](const char *arg) {
            controlPlaneConfig_ = std::filesystem::path(arg);
            if (!std::filesystem::exists(controlPlaneConfig_.value())) {
                ::error("%1% does not exist. Please provide a valid file path.",
                        controlPlaneConfig_.value().c_str());
                return false;
            }
            return true;
        },
        "The path to the control plane configuration file. The instructions in this file will be "
        "converted into a semantic representation and merged with the representation of the P4 "
        "file.");
    registerOption(
        "--print-performance-report", nullptr,
        [](const char *) {
            enablePerformanceLogging();
            return true;
        },
        "Print timing report summary at the end of the program.");
    registerOption(
        "--server-mode", nullptr,
        [this](const char *) {
            serverMode_ = true;
            return true;
        },
        "Toogle Flay's server mode and start a P4Runtime server.");
    registerOption(
        "--server-address", "serverAddress",
        [this](const char *arg) {
            if (!serverMode_) {
                ::warning(
                    "Server mode was not active but a server address was provided. Enabling server "
                    "mode.");
                serverMode_ = true;
            }
            serverAddress_ = arg;
            return true;
        },
        "The address of the Flay service in the format ADDRESS:PORT.");
    registerOption(
        "--config-update-pattern", "configUpdatePattern",
        [this](const char *arg) {
            configUpdatePattern_ = arg;
            return true;
        },
        "A pattern which can either match a single file or a list of files. Primarily used for "
        "testing.");
    registerOption(
        "--use-placeholders", nullptr,
        [this](const char *) {
            usePlaceholders_ = true;
            return true;
        },
        "Use placeholders instead of symbolic variables for entities affected by recirculation or "
        "cloning.");
    registerOption(
        "--strict", nullptr,
        [this](const char *) {
            strict_ = true;
            return true;
        },
        "In strict mode, Flay will report error upon adding more reachability condition for "
        "existing nodes");
    registerOption(
        "--optimized-output-dir", "optimizedOutputDir",
        [this](const char *arg) {
            optimizedOutputDir_ = std::filesystem::path(arg);
            return true;
        },
        "The path to the output directory of the optimized P4 program(s).");
    registerOption(
        "--preserve-data-plane-variables", nullptr,
        [this](const char *) {
            collapseDataPlaneOperations_ = false;
            return true;
        },
        "Preserve arithmetic operations on variables sourced from the data plane (e.g., header "
        "reads). Flay "
        "will otherwise collapse these operations only perform live-variable analysis on "
        "control-plane sourced variables.");
    registerOption(
        "--generate-p4info", "filePath",
        [this](const char *arg) {
            p4InfoFilePath = arg;
            if (p4InfoFilePath.value().extension() != ".txtpb") {
                ::error("%1% must have a .txtpb extension.", p4InfoFilePath.value().c_str());
                return false;
            }
            return true;
        },
        "Write the P4Runtime control plane API description (P4Info) to the specified .txtpb file.");
}

std::filesystem::path FlayOptions::getControlPlaneConfig() const {
    return controlPlaneConfig_.value();
}

bool FlayOptions::serverModeActive() const { return serverMode_; }

std::string FlayOptions::getServerAddress() const { return serverAddress_; }

bool FlayOptions::hasControlPlaneConfig() const { return controlPlaneConfig_.has_value(); }

bool FlayOptions::hasConfigurationUpdatePattern() const { return configUpdatePattern_.has_value(); }

std::string FlayOptions::getConfigurationUpdatePattern() const {
    return configUpdatePattern_.value();
}

bool FlayOptions::usePlaceholders() const { return usePlaceholders_; }

bool FlayOptions::isStrict() const { return strict_; }

std::optional<std::filesystem::path> FlayOptions::getOptimizedOutputDir() const {
    return optimizedOutputDir_;
}

bool FlayOptions::collapseDataPlaneOperations() const { return collapseDataPlaneOperations_; }

std::optional<std::string> FlayOptions::getP4InfoFilePath() const { return p4InfoFilePath; }

}  // namespace P4Tools
