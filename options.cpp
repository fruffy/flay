#include "backends/p4tools/modules/flay/options.h"

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/options.h"
#include "backends/p4tools/modules/flay/toolname.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace P4Tools {

FlayOptions &FlayOptions::get() {
    static FlayOptions INSTANCE;
    return INSTANCE;
}

const char *FlayOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for Flay.");
}

const std::set<std::string> K_SUPPORTED_CONTROL_PLANES = {"P4RUNTIME", "BFRUNTIME"};

FlayOptions::FlayOptions(const std::string &message)
    : AbstractP4cToolOptions(Flay::TOOL_NAME, message) {
    registerOption(
        "--config-file", "controlPlaneConfig",
        [this](const char *arg) {
            _controlPlaneConfig = std::filesystem::path(arg);
            if (!std::filesystem::exists(_controlPlaneConfig.value())) {
                ::error("%1% does not exist. Please provide a valid file path.",
                        _controlPlaneConfig.value().c_str());
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
            _serverMode = true;
            return true;
        },
        "Toogle Flay's server mode and start a P4Runtime server.");
    registerOption(
        "--server-address", "serverAddress",
        [this](const char *arg) {
            if (!_serverMode) {
                ::warning(
                    "Server mode was not active but a server address was provided. Enabling server "
                    "mode.");
                _serverMode = true;
            }
            _serverAddress = arg;
            return true;
        },
        "The address of the Flay service in the format ADDRESS:PORT.");
    registerOption(
        "--config-update-pattern", "configUpdatePattern",
        [this](const char *arg) {
            _configUpdatePattern = arg;
            return true;
        },
        "A pattern which can either match a single file or a list of files. Primarily used for "
        "testing.");
    registerOption(
        "--use-placeholders", nullptr,
        [this](const char *) {
            _usePlaceholders = true;
            return true;
        },
        "Use placeholders instead of symbolic variables for entities affected by recirculation or "
        "cloning.");
    registerOption(
        "--strict", nullptr,
        [this](const char *) {
            _strict = true;
            return true;
        },
        "In strict mode, Flay will report error upon adding more reachability condition for "
        "existing nodes");
    registerOption(
        "--optimized-output-dir", "optimizedOutputDir",
        [this](const char *arg) {
            _optimizedOutputDir = std::filesystem::path(arg);
            return true;
        },
        "The path to the output directory of the optimized P4 program(s).");
    registerOption(
        "--collapse-data-plane-variables", nullptr,
        [this](const char *) {
            _collapseDataPlaneOperations = true;
            return true;
        },
        "Preserve arithmetic operations on variables sourced from the data plane (e.g., header "
        "reads). Flay "
        "will otherwise collapse these operations only perform live-variable analysis on "
        "control-plane sourced variables.");
    registerOption(
        "--user-p4info", "filePath",
        [this](const char *arg) {
            _userP4Info = arg;
            if (!std::filesystem::exists(_userP4Info.value())) {
                ::error("%1% does not exist. Please provide a valid file path.",
                        _userP4Info.value().c_str());
                return false;
            }
            if (_p4InfoFilePath.has_value()) {
                ::error(
                    "Both --user-p4info and --generate-p4info are specified. Please specify only "
                    "one.");
                return false;
            }
            return true;
        },
        "Write the P4Runtime control plane API description (P4Info) to the specified .txtpb file.");
    registerOption(
        "--generate-p4info", "filePath",
        [this](const char *arg) {
            _p4InfoFilePath = arg;
            if (_p4InfoFilePath.value().extension() != ".txtpb") {
                ::error("%1% must have a .txtpb extension.", _p4InfoFilePath.value().c_str());
                return false;
            }
            if (_userP4Info.has_value()) {
                ::error(
                    "Both --user-p4info and --generate-p4info are specified. Please specify only "
                    "one.");
                return false;
            }
            return true;
        },
        "Write the P4Runtime control plane API description (P4Info) to the specified .txtpb file.");
    registerOption(
        "--control-plane", "controlPlaneApi",
        [this](const char *arg) {
            _controlPlaneApi = arg;
            transform(_controlPlaneApi.begin(), _controlPlaneApi.end(), _controlPlaneApi.begin(),
                      ::toupper);
            return true;
            if (K_SUPPORTED_CONTROL_PLANES.find(_controlPlaneApi) ==
                K_SUPPORTED_CONTROL_PLANES.end()) {
                ::error(
                    "Test back end %1% not implemented for this target. Supported back ends are "
                    "%2%.",
                    _controlPlaneApi, Utils::containerToString(K_SUPPORTED_CONTROL_PLANES));
                return false;
            }
        },
        "Specifies the control plane API to use. Defaults to P4Rtuntime.");
    registerOption(
        "--skip-parsers", nullptr,
        [this](const char *) {
            _skipParsers = true;
            return true;
        },
        "Skip parsers in the analysis and replace the parser output result with symbolic "
        "variables.");
}

std::filesystem::path FlayOptions::controlPlaneConfig() const {
    return _controlPlaneConfig.value();
}

bool FlayOptions::serverModeActive() const { return _serverMode; }

std::string_view FlayOptions::serverAddress() const { return _serverAddress; }

bool FlayOptions::hasControlPlaneConfig() const { return _controlPlaneConfig.has_value(); }

bool FlayOptions::hasConfigurationUpdatePattern() const { return _configUpdatePattern.has_value(); }

std::string_view FlayOptions::configurationUpdatePattern() const {
    return _configUpdatePattern.value();
}

bool FlayOptions::usePlaceholders() const { return _usePlaceholders; }

bool FlayOptions::isStrict() const { return _strict; }

std::optional<std::filesystem::path> FlayOptions::optimizedOutputDir() const {
    return _optimizedOutputDir;
}

bool FlayOptions::collapseDataPlaneOperations() const { return _collapseDataPlaneOperations; }

std::optional<std::filesystem::path> FlayOptions::p4InfoFilePath() const { return _p4InfoFilePath; }

std::optional<std::filesystem::path> FlayOptions::userP4Info() const { return _userP4Info; }

std::string_view FlayOptions::controlPlaneApi() const { return _controlPlaneApi; }

bool FlayOptions::skipParsers() const { return _skipParsers; }

}  // namespace P4Tools
