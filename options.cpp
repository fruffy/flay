#include "backends/p4tools/modules/flay/options.h"

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/options.h"
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

FlayOptions::FlayOptions(const std::string &message) : AbstractP4cToolOptions(message) {
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

}  // namespace P4Tools
