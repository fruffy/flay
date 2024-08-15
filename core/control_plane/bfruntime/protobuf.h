#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_BFRUNTIME_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_BFRUNTIME_PROTOBUF_H_

#include <fcntl.h>

#include <google/protobuf/text_format.h>

#include <optional>

#include "control-plane/p4RuntimeArchHandler.h"
#include "ir/ir.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/common/control_plane/bfruntime/bfruntime.pb.h"
#include "backends/p4tools/modules/flay/core/control_plane/bfruntime/flaytests.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/core/control_plane/symbols.h"

/// Converts a Protobuf object and the instructions contained
/// within into P4C-IR nodes. These IR-nodes are structured to represent a
/// control-plane configuration that maps to the semantic data-plane
/// representation of the program.
namespace P4::P4Tools::Flay::BfRuntime {

/// Convert a Protobuf BFRuntime entity object into a set of IR-based
/// control-plane constraints. Use the
/// @param irToIdMap to lookup the nodes associated with BFRuntime Ids.
/// @param symbolSet tracks the symbols used in this conversion.
[[nodiscard]] int updateControlPlaneConstraintsWithEntityMessage(
    const bfrt_proto::Entity &entity, const p4::config::v1::P4Info &p4Info,
    ControlPlaneConstraints &controlPlaneConstraints, const ::bfrt_proto::Update_Type &updateType,
    SymbolSet &symbolSet);

/// Convert a Protobuf Config object into a set of IR-based control-plane
/// constraints. Use the
/// @param irToIdMap to lookup the nodes associated with BFRuntime Ids.
/// @param symbolSet tracks the symbols used in this conversion.
[[nodiscard]] int updateControlPlaneConstraints(
    const bfruntime::flaytests::Config &protoControlPlaneConfig,
    const p4::config::v1::P4Info &p4Info, ControlPlaneConstraints &controlPlaneConstraints,
    SymbolSet &symbolSet);

}  // namespace P4::P4Tools::Flay::BfRuntime

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_BFRUNTIME_PROTOBUF_H_ */
