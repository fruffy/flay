#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"

#include "control-plane/p4RuntimeSymbolTable.h"

namespace P4Tools::Flay {

P4RuntimeIdtoIrNodeMap P4RuntimeToIRMapper::getP4RuntimeIdtoIrNodeMap() const { return idToIrMap; }

bool P4RuntimeToIRMapper::preorder(const IR::P4Table *table) {
    if (P4::ControlPlaneAPI::isHidden(table)) {
        return false;
    }
    auto p4RuntimeId = p4RuntimeMaps.lookupP4RuntimeId(table->controlPlaneName());
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), table);
    } else {
        ::error("Table %1% not found in the P4Runtime ID map.");
    }

    return false;
}

bool P4RuntimeToIRMapper::preorder(const IR::Type_Header *hdr) {
    if (!P4::ControlPlaneAPI::isControllerHeader(hdr) || P4::ControlPlaneAPI::isHidden(hdr)) {
        return false;
    }
    auto p4RuntimeId = p4RuntimeMaps.lookupP4RuntimeId(hdr->controlPlaneName());
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), hdr);
    } else {
        ::error("Header %1% not found in the P4Runtime ID map.");
    }
    return false;
}

bool P4RuntimeToIRMapper::preorder(const IR::P4ValueSet *valueSet) {
    if (P4::ControlPlaneAPI::isHidden(valueSet)) {
        return false;
    }
    auto p4RuntimeId = p4RuntimeMaps.lookupP4RuntimeId(valueSet->controlPlaneName());
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), valueSet);
    } else {
        ::error("Value set %1% not found in the P4Runtime ID map.");
    }
    return false;
}

bool P4RuntimeToIRMapper::preorder(const IR::P4Action *action) {
    if (P4::ControlPlaneAPI::isHidden(action)) {
        return false;
    }
    auto p4RuntimeId = p4RuntimeMaps.lookupP4RuntimeId(action->controlPlaneName());
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), action);
    } else {
        ::error("P4 action %1% not found in the P4Runtime ID map.");
    }
    return false;
}

}  // namespace P4Tools::Flay
