#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_P4RUNTIME_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_P4RUNTIME_PROTOBUF_H_

#include <optional>

#include "control-plane/p4RuntimeArchHandler.h"
#include "ir/ir.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/modules/flay/core/control_plane/p4runtime/flaytests.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/control_plane/symbols.h"

/// Parses a Protobuf text message file and converts the instructions contained
/// within into P4C-IR nodes. These IR-nodes are structured to represent a
/// control-plane configuration that maps to the semantic data-plane
/// representation of the program.
namespace P4::P4Tools::Flay::P4Runtime {

/// Convert a Protobuf P4Runtime entity object into a set of IR-based
/// control-plane constraints. Use the
/// @param symbolSet tracks the symbols used in this conversion.
[[nodiscard]] int updateControlPlaneConstraintsWithEntityMessage(
    const p4::v1::Entity &entity, const p4::config::v1::P4Info &p4Info,
    ControlPlaneConstraints &controlPlaneConstraints, const ::p4::v1::Update_Type &updateType,
    SymbolSet &symbolSet);

/// Convert a Protobuf Config object into a set of IR-based control-plane
/// constraints. Use the
/// @param irToIdMap to lookup the nodes associated with P4Runtime Ids.
/// @param symbolSet tracks the symbols used in this conversion.
[[nodiscard]] int updateControlPlaneConstraints(
    const ::p4runtime::flaytests::Config &protoControlPlaneConfig,
    const p4::config::v1::P4Info &p4Info, ControlPlaneConstraints &controlPlaneConstraints,
    SymbolSet &symbolSet);

}  // namespace P4::P4Tools::Flay::P4Runtime

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_P4RUNTIME_PROTOBUF_H_ */
