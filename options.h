#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_

#include <filesystem>
#include <optional>

#include "backends/p4tools/common/options.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for P4Testgen.
class FlayOptions : public AbstractP4cToolOptions {
 public:
    FlayOptions(const FlayOptions &) = delete;

    FlayOptions(FlayOptions &&) = delete;

    FlayOptions &operator=(const FlayOptions &) = delete;

    FlayOptions &operator=(FlayOptions &&) = delete;

    virtual ~FlayOptions() = default;

    /// @returns the singleton instance of this class.
    static FlayOptions &get();

    const char *getIncludePath() override;

    /// @returns the path set with --config-file.
    [[nodiscard]] std::filesystem::path getControlPlaneConfig() const;

    /// @returns true when the --config-file option has been set.
    [[nodiscard]] bool hasControlPlaneConfig() const;

    /// @returns true when the user would like to run in server mode.
    [[nodiscard]] bool serverModeActive() const;

    /// @returns the server address set with --server-address.
    [[nodiscard]] std::string getServerAddress() const;

    /// @returns true when the --config-update-pattern option has been set.
    [[nodiscard]] bool hasConfigurationUpdatePattern() const;

    /// @returns the configuration update pattern set with --config-update-pattern.
    [[nodiscard]] std::string getConfigurationUpdatePattern() const;

    /// @returns true when the --use-placeholders option has been set.
    [[nodiscard]] bool usePlaceholders() const;

    /// @returns true when the --strict option has been set.
    [[nodiscard]] bool isStrict() const;

    /// @returns the path set with --optimized-output-file.
    [[nodiscard]] std::optional<std::filesystem::path> getOptimizedOutputFile() const;

    /// @returns false when --preserve-data-plane-variables has been set.
    [[nodiscard]] bool collapseDataPlaneOperations() const;

    /// @returns the path set with --p4-info-file.
    [[nodiscard]] std::optional<std::string> getP4InfoFilePath() const;

 protected:
    explicit FlayOptions(
        const std::string &message = "Remove control-plane dead code from a P4 program.");

 private:
    /// Path to the initial control plane configuration file.
    std::optional<std::filesystem::path> controlPlaneConfig_ = std::nullopt;

    /// Toggle server mode.
    /// After parsing, Flay can initialize a P4Runtime server which handles control-plane messages.
    bool serverMode_ = false;

    /// Server address for the Flay service.
    std::string serverAddress_ = "localhost:50051";

    /// A pattern which can either match a single file or a list of files.
    /// This pattern is converted into a list of configuration updates.
    /// Used for testing.
    std::optional<std::string> configUpdatePattern_;

    /// Toggle use of placeholder variables to model recirculated or cloned packets.
    bool usePlaceholders_ = false;

    /// In strict mode, Flay will report errors instead of warnings for certain unsafe behavior.
    /// For example, when adding more than one reachability condition for one IR node.
    bool strict_ = false;

    /// The path to the output file of the optimized P4 program.
    std::optional<std::filesystem::path> optimizedOutputFile_ = std::nullopt;

    /// Collapse arithmetic operations on data plane variables.
    bool collapseDataPlaneOperations_ = true;

    // Write the P4Runtime control plane API description to the specified file.
    std::optional<std::filesystem::path> p4InfoFilePath = std::nullopt;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_ */
