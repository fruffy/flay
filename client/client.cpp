#include <cstdlib>
#include <iomanip>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"

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
    return ProtobufDeserializer::parseEntity(MESSAGE);
}

// printInfo("Sending an entry update...\n");
// auto entity = createMiddleblockUpdate();
// if (!entity.has_value()) {
//     return EXIT_FAILURE;
// }
// service.processUpdateMessage(entity.value());

int main(int argc, char *argv[]) { return EXIT_SUCCESS; }
