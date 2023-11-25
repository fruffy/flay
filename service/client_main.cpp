#include <cstdlib>

#include "backends/p4tools/modules/flay/lib/logging.h"
#include "backends/p4tools/modules/flay/service/client_options.h"
#include "backends/p4tools/modules/flay/service/flay_client.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"

#pragma GCC diagnostic pop

namespace P4Tools::Flay {

std::optional<p4::v1::Entity> createMiddleblockUpdate() {
    static std::string MESSAGE(R"""(
    table_entry {
      table_id: 33554503
      priority: 1
      # Match field dst_mac
      match {
        field_id: 1
        ternary {
          value: "\x2C\xA2\x80\x0E\xE2\xCC"
          mask: "\x00\x00\x00\x00\x00\x00"
        }
      }
      # Match field in_port
      match {
        field_id: 2
        ternary {
          value: "\x01\xD2"
          mask: "\x00\x00"
        }
      }
      # Action ingress.l3_admit.admit_to_l3
      action {
        action {
          action_id: 16777224
        }
      }
    }
  )""");
    return FlayClient::parseEntity(MESSAGE);
}

int run(const FlayClientOptions &options) {
    enableInformationLogging();
    // Create a channel to the server
    auto channel =
        grpc::CreateChannel(options.getServerAddress(), grpc::InsecureChannelCredentials());
    FlayClient client(channel);

    auto entity = createMiddleblockUpdate();
    if (!entity.has_value()) {
        return EXIT_FAILURE;
    }
    client.sendWriteRequest(entity.value(), p4::v1::Update_Type::Update_Type_MODIFY);
    printInfo("Done sending requests.");
    return EXIT_SUCCESS;
}

}  // namespace P4Tools::Flay

class FlayClientContext : public BaseCompileContext {};

int main(int argc, char *argv[]) {
    // Set up the compilation context.
    AutoCompileContext autoP4FlayClientContext(new FlayClientContext);
    P4Tools::Flay::FlayClientOptions options("A client to communicate with a Flay server.");

    // Process command-line options.
    // TODO: Error handling?
    options.process(argc, argv);

    P4Tools::Flay::run(options);

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
