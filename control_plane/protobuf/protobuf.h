#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_

#include <filesystem>

#include "ir/ir.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/modules/flay/control_plane/protobuf/flaytests.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"

namespace P4Tools::Flay {

/// Parses a Protobuf text message file and converts the instructions contained within into P4C-IR
/// nodes. These IR-nodes are structure to represent a control-plane configuration that maps to the
/// semantic data-plane representation of the program.
class ProtobufDeserializer {
 private:
    /// Convert a P4Runtime TableAction into the appropriate symbolic constraint assignments.
    static void convertTableAction(const p4::v1::Action &tblAction, cstring tableName,
                                   const IR::P4Action &p4Action,
                                   ControlPlaneConstraints &controlPlaneConstraints);

    /// Convert a P4Runtime FieldMatch into the appropriate symbolic constraint assignments.
    static void convertTableMatch(const p4::v1::FieldMatch &field, cstring tableName,
                                  cstring keyFieldName, const IR::Expression &keyExpr,
                                  ControlPlaneConstraints &controlPlaneConstraints);

    /// Convert a P4Runtime TableEntry into the appropriate symbolic constraint assignments.
    static void convertTableEntry(const P4RuntimeIdtoIrNodeMap &irToIdMap,
                                  const p4::v1::TableEntry &tableEntry,
                                  ControlPlaneConstraints &controlPlaneConstraints);

 public:
    /// Deserialize a .proto file into a P4Runtime-compliant Protobuf object.
    static flaytests::Config deserializeProtobufConfig(std::filesystem::path inputFile);

    /// Convert a Protobuf Config object into a set of IR-based control-plane constraints. Use the
    /// @param irToIdMap to lookup the nodes associated with P4Runtime Ids.
    static ControlPlaneConstraints convertToControlPlaneConstraints(
        const flaytests::Config &protoControlPlaneConfig, const P4RuntimeIdtoIrNodeMap &irToIdMap);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_ */
