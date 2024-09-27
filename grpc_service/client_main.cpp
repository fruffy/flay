#include <cstdlib>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/grpc_service/client_options.h"
#include "backends/p4tools/modules/flay/grpc_service/flay_client.h"
#include "lib/error.h"

namespace P4::P4Tools::Flay {

void run(const FlayClientOptions &options) {
    enableInformationLogging();
    // Create a channel to the server
    auto channel =
        grpc::CreateChannel(options.getServerAddress(), grpc::InsecureChannelCredentials());
    FlayClient client(channel);

    for (const auto &update : options.getProtoUpdates()) {
        auto request = FlayClient::parseWriteRequestFile(update);
        if (!request.has_value()) {
            return;
        }
        auto result = client.sendWriteRequest(request.value());
        if (!result.ok()) {
            error("Code %1% - Failed to send write request. Reason: %2%", result.error_code(),
                  result.error_message());
            return;
        }
    }
    printInfo("Done sending requests.");
    printPerformanceReport();
}

}  // namespace P4::P4Tools::Flay

class FlayClientContext : public P4::BaseCompileContext {};

int main(int argc, char *argv[]) {
    // Set up the compilation context and the options.
    P4::AutoCompileContext autoP4FlayClientContext(new FlayClientContext);
    P4::P4Tools::Flay::FlayClientOptions options("A Flay gRPC client.");

    // Process command-line options.
    options.process(argc, argv);
    if (P4::errorCount() != 0) {
        return EXIT_FAILURE;
    }

    // Run the Flay client.
    P4::P4Tools::Flay::run(options);
    return P4::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
