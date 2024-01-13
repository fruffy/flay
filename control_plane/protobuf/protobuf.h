#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_

#include <fcntl.h>

#include <google/protobuf/text_format.h>

#include <filesystem>
#include <optional>

#include "control-plane/p4RuntimeArchHandler.h"
#include "ir/ir.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/modules/flay/control_plane/protobuf/flaytests.pb.h"
#pragma GCC diagnostic pop

#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"

namespace P4Tools::Flay {

/// Parses a Protobuf text message file and converts the instructions contained
/// within into P4C-IR nodes. These IR-nodes are structured to represent a
/// control-plane configuration that maps to the semantic data-plane
/// representation of the program.
class ProtobufDeserializer {
 private:
    /// Helper function, which converts a Protobuf byte string into a big integer
    /// (boost cpp_int).
    [[nodiscard]] static big_int protoValueToBigInt(const std::string &valueString);

    /// Convert a P4Runtime TableAction into the appropriate symbolic constraint
    /// assignments.
    [[nodiscard]] static std::optional<const IR::Expression *> convertTableAction(
        const p4::v1::Action &tblAction, cstring tableName, const p4::config::v1::Action &p4Action,
        SymbolSet &symbolSet);

    /// Convert a P4Runtime FieldMatch into the appropriate symbolic constraint
    /// assignments.
    /// @param symbolSet tracks the symbols used in this conversion.
    [[nodiscard]] static std::optional<TableKeySet> produceTableMatch(
        const p4::v1::FieldMatch &field, cstring tableName,
        const p4::config::v1::MatchField &matchField, SymbolSet &symbolSet);

    /// Convert a P4Runtime TableEntry into a TableMatchEntry.
    /// Returns std::nullopt if the conversion fails.
    /// @param symbolSet tracks the symbols used in this conversion.
    [[nodiscard]] static std::optional<TableMatchEntry *> produceTableEntry(
        cstring tableName, P4::ControlPlaneAPI::p4rt_id_t tblId,
        const p4::config::v1::P4Info &p4Info, const p4::v1::TableEntry &tableEntry,
        SymbolSet &symbolSet);

    /// Convert a P4Runtime TableEntry into the appropriate symbolic constraint
    /// assignments.
    /// @param symbolSet tracks the symbols used in this conversion.
    [[nodiscard]] static int updateTableEntry(const p4::config::v1::P4Info &p4Info,
                                              const p4::v1::TableEntry &tableEntry,
                                              ControlPlaneConstraints &controlPlaneConstraints,
                                              const ::p4::v1::Update_Type &updateType,
                                              SymbolSet &symbolSet);

 public:
    /// Convert a Protobuf P4Runtime entity object into a set of IR-based
    /// control-plane constraints. Use the
    /// @param irToIdMap to lookup the nodes associated with P4Runtime Ids.
    /// @param symbolSet tracks the symbols used in this conversion.
    [[nodiscard]] static int updateControlPlaneConstraintsWithEntityMessage(
        const p4::v1::Entity &entity, const p4::config::v1::P4Info &p4Info,
        ControlPlaneConstraints &controlPlaneConstraints, const ::p4::v1::Update_Type &updateType,
        SymbolSet &symbolSet);

    /// Convert a Protobuf Config object into a set of IR-based control-plane
    /// constraints. Use the
    /// @param irToIdMap to lookup the nodes associated with P4Runtime Ids.
    /// @param symbolSet tracks the symbols used in this conversion.
    [[nodiscard]] static int updateControlPlaneConstraints(
        const flaytests::Config &protoControlPlaneConfig, const p4::config::v1::P4Info &p4Info,
        ControlPlaneConstraints &controlPlaneConstraints, SymbolSet &symbolSet);

    /// Deserialize a .proto file into a P4Runtime-compliant Protobuf object.
    template <class T>
    [[nodiscard]] static std::optional<T> deserializeProtoObjectFromFile(
        const std::filesystem::path &inputFile) {
        T protoObject;

        // Parse the input file into the Protobuf object.
        int fd = open(inputFile.c_str(), O_RDONLY);  // NOLINT, we are forced to use open here.
        RETURN_IF_FALSE_WITH_MESSAGE(fd > 0, std::nullopt,
                                     ::error("Failed to open file %1%", inputFile.c_str()));
        google::protobuf::io::ZeroCopyInputStream *input =
            new google::protobuf::io::FileInputStream(fd);

        RETURN_IF_FALSE_WITH_MESSAGE(google::protobuf::TextFormat::Parse(input, &protoObject),
                                     std::nullopt,
                                     ::error("Failed to parse configuration \"%1%\" for file %2%",
                                             protoObject.ShortDebugString(), inputFile.c_str()));

        printInfo("Parsed configuration: %1%", protoObject.DebugString());
        // Close the open file.
        close(fd);
        return protoObject;
    }
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_PROTOBUF_PROTOBUF_H_ */
