#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_

#include <filesystem>

#include "backends/p4tools/common/options.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for P4Testgen.
class FlayOptions : public AbstractP4cToolOptions {
    /// List of the supported config file extensions (and with that, formats).
    static const std::set<std::string> SUPPORTED_CONFIG_EXTENSIONS;

 public:
    virtual ~FlayOptions() = default;

    /// @returns the singleton instance of this class.
    static FlayOptions &get();

    const char *getIncludePath() override;

    /// @returns the path set with --config-file.
    std::filesystem::path getControlPlaneConfig() const;

    /// @returns true when the --config-file option has been set.
    bool hasControlPlaneConfig() const;

 private:
    /// Path to the initial control plane configuration file.
    std::optional<std::filesystem::path> controlPlaneConfig = std::nullopt;

    FlayOptions();
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_ */
