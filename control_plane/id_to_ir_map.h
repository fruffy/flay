#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_ID_TO_IR_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_ID_TO_IR_MAP_H_

#include <map>

#include "backends/p4tools/common/control_plane/p4runtime_api.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "ir/ir.h"

namespace P4Tools::Flay {

/// The mapping from P4Runtime IDs to IR nodes. Currently a simple std::map.
using P4RuntimeIdtoIrNodeMap = std::map<P4::ControlPlaneAPI::p4rt_id_t, const IR::IDeclaration *>;

/// Maps P4Runtime IDs (https://p4.org/p4-spec/p4runtime/main/P4Runtime-Spec.html#sec-id-allocation)
/// to their respective IR node. This is useful when parsing control plane configuration messages
/// that use the P4Runtime protocol. These message do only specify the P4Runtime ID and do not
/// contain typing or naming information on the related IR node.
class P4RuntimeToIRMapper : public Inspector {
 private:
    /// A mapping from P4 control plane names to their mapped P4Runtime ids and vice versa.
    P4::ControlPlaneAPI::P4RuntimeMaps p4RuntimeMaps;

    /// The mapping from P4Runtime IDs to IR nodes.
    P4RuntimeIdtoIrNodeMap idToIrMap;

    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Type_Header *hdr) override;
    bool preorder(const IR::P4ValueSet *valueSet) override;
    bool preorder(const IR::P4Action *action) override;

 public:
    explicit P4RuntimeToIRMapper(const p4::config::v1::P4Info &p4Info)
        : p4RuntimeMaps(P4::ControlPlaneAPI::P4RuntimeMaps(p4Info)) {}

    /// @returns the mapping from P4Runtime IDs to IR nodes.
    [[nodiscard]] P4RuntimeIdtoIrNodeMap getP4RuntimeIdtoIrNodeMap() const;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_ID_TO_IR_MAP_H_ */
