#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_ID_TO_IR_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_ID_TO_IR_MAP_H_

#include <map>

#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "ir/ir.h"

namespace P4Tools::Flay {

using P4RuntimeIDtoIRObjectMap = std::map<P4::ControlPlaneAPI::p4rt_id_t, const IR::IDeclaration *>;

class MapP4RuntimeIdtoIR : public Inspector {
 private:
    P4RuntimeIDtoIRObjectMap idToIrMap;

    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Type_Header *hdr) override;
    bool preorder(const IR::P4ValueSet *valueSet) override;
    bool preorder(const IR::P4Action *action) override;

 public:
    P4RuntimeIDtoIRObjectMap getP4RuntimeIDtoIRObjectMap() const;

    MapP4RuntimeIdtoIR() = default;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_ID_TO_IR_MAP_H_ */
