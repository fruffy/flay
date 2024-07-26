#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_

#include <filesystem>
#include <optional>

#include "backends/p4tools/common/options.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for P4Testgen.
class FlayOptions : public AbstractP4cToolOptions {
 public:
    FlayOptions();
    ~FlayOptions() override = default;

    /// @returns the singleton instance of this class.
    static FlayOptions &get();

    static void set(const FlayOptions &options) { get() = options; }

    /// @returns the path set with --config-file.
    [[nodiscard]] std::filesystem::path controlPlaneConfig() const;

    /// @returns true when the --config-file option has been set.
    [[nodiscard]] bool hasControlPlaneConfig() const;

    /// @returns true when the user would like to run in server mode.
    [[nodiscard]] bool serverModeActive() const;

    /// @returns the server address set with --server-address.
    [[nodiscard]] std::string_view serverAddress() const;

    /// @returns true when the --config-update-pattern option has been set.
    [[nodiscard]] bool hasConfigurationUpdatePattern() const;

    /// @returns the configuration update pattern set with --config-update-pattern.
    [[nodiscard]] std::string_view configurationUpdatePattern() const;

    /// @returns true when the --use-placeholders option has been set.
    [[nodiscard]] bool usePlaceholders() const;

    /// @returns true when the --strict option has been set.
    [[nodiscard]] bool isStrict() const;

    /// @returns the path set with --optimized-output-dir.
    [[nodiscard]] std::optional<std::filesystem::path> optimizedOutputDir() const;

    /// @returns true when --collapse-data-plane-variables has been set.
    [[nodiscard]] bool collapseDataPlaneOperations() const;

    /// @returns the path set with --generate-p4info.
    [[nodiscard]] std::optional<std::filesystem::path> p4InfoFilePath() const;

    /// @returns the path to the user p4 info file set by the --user-p4info option.
    [[nodiscard]] std::optional<std::filesystem::path> userP4Info() const;

    /// @returns the control plane API to use.
    [[nodiscard]] std::string_view controlPlaneApi() const;

    /// @returns true when the --skip-parsers option has been set.
    [[nodiscard]] bool skipParsers() const;

    /// @returns true when the --skip-side-effect-ordering option has been set.
    [[nodiscard]] bool skipSideEffectOrdering() const;

    /// Sets the path to the initial control plane configuration file.
    void setControlPlaneConfig(const std::filesystem::path &path) { _controlPlaneConfig = path; }

    /// Sets the server mode.
    void setServerMode() { _serverMode = true; }

    /// Sets the server address.
    void setServerAddress(const std::string &address) { _serverAddress = address; }

    /// Sets the configuration update pattern.
    void setConfigurationUpdatePattern(const std::string &pattern) {
        _configUpdatePattern = pattern;
    }

    /// Sets the use of placeholders.
    void setUsePlaceholders() { _usePlaceholders = true; }

    /// Sets the strict mode.
    void setStrict() { _strict = true; }

    /// Sets the path to the output file of the optimized P4 program.
    void setOptimizedOutputDir(const std::filesystem::path &path) { _optimizedOutputDir = path; }

    /// Sets the path to the user p4 info file.
    void setP4InfoFilePath(const std::filesystem::path &path) { _p4InfoFilePath = path; }

    /// Sets the control plane API to use.
    void setControlPlaneApi(const std::string &api) { _controlPlaneApi = api; }

    /// Sets the path to the user p4 info file.
    void setUserP4Info(const std::filesystem::path &path) { _userP4Info = path; }

    /// Set whether to skip parsers in the analysis and replace the parser output result with
    /// symbolic variables.
    void setSkipParsers() { _skipParsers = true; }

    /// Set whether to skip side-effect ordering in the front end.
    void setSkipSideEffectOrdering() { _skipSideEffectOrdering = true; }

 private:
    /// Path to the initial control plane configuration file.
    std::optional<std::filesystem::path> _controlPlaneConfig = std::nullopt;

    /// Toggle server mode.
    /// After parsing, Flay can initialize a P4Runtime server which handles control-plane messages.
    bool _serverMode = false;

    /// Server address for the Flay service.
    std::string _serverAddress = "localhost:50051";

    /// A pattern which can either match a single file or a list of files.
    /// This pattern is converted into a list of configuration updates.
    /// Used for testing.
    std::optional<std::string> _configUpdatePattern;

    /// Toggle use of placeholder variables to model recirculated or cloned packets.
    bool _usePlaceholders = false;

    /// In strict mode, Flay will report errors instead of warnings for certain unsafe behavior.
    /// For example, when adding more than one reachability condition for one IR node.
    bool _strict = false;

    /// The path to the output file of the optimized P4 program.
    std::optional<std::filesystem::path> _optimizedOutputDir = std::nullopt;

    /// Collapse arithmetic operations on data plane variables.
    bool _collapseDataPlaneOperations = false;

    // Use a user-supplied P4Info file instead of generating one.
    std::optional<std::filesystem::path> _userP4Info = std::nullopt;

    // Write the P4Runtime control plane API description to the specified file.
    std::optional<std::filesystem::path> _p4InfoFilePath = std::nullopt;

    // The control plane API to use. Defaults to P4Runtime.
    std::string _controlPlaneApi = "P4RUNTIME";

    /// Skip parsers in the analysis and replace the parser output result with symbolic variables.
    bool _skipParsers = false;

    /// Skip side-effect ordering in the front end.
    bool _skipSideEffectOrdering = false;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_ */
