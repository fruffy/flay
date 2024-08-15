#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_GRPC_SERVICE_CLIENT_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_GRPC_SERVICE_CLIENT_OPTIONS_H_

#include <filesystem>
#include <string>
#include <vector>

#include "lib/options.h"

namespace P4::P4Tools::Flay {

/// Encapsulates and processes command-line options for a compiler-based tool. Implementations
/// should use the singleton pattern and define a static get() for obtaining the singleton
/// instance.
class FlayClientOptions : protected Util::Options {
 public:
    explicit FlayClientOptions(std::string_view message);

    FlayClientOptions(const FlayClientOptions &) = default;
    FlayClientOptions(FlayClientOptions &&) = delete;
    FlayClientOptions &operator=(const FlayClientOptions &) = default;
    FlayClientOptions &operator=(FlayClientOptions &&) = delete;
    virtual ~FlayClientOptions() = default;

    const char *getIncludePath() const override;

    std::vector<const char *> *process(int argc, char *const *argv) override;

    /// @returns true when the user would like to run in server mode.
    [[nodiscard]] bool serverModeActive() const;

    /// @returns the server address set with --server-address.
    [[nodiscard]] std::string getServerAddress() const;

    /// @returns the list of proto updates which are to be sent to the server.
    [[nodiscard]] std::vector<std::filesystem::path> getProtoUpdates() const;

 private:
    /// Server address for the Flay service.
    std::string serverAddress = "localhost:50051";

    /// The list of proto updates which are to be sent to the server.
    std::vector<std::filesystem::path> protoUpdates;
};

}  // namespace P4::P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_GRPC_SERVICE_CLIENT_OPTIONS_H_ */
