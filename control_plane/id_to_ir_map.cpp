#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"

#include "control-plane/p4RuntimeSymbolTable.h"

namespace P4Tools::Flay {

P4RuntimeIDtoIRObjectMap MapP4RuntimeIdtoIR::getP4RuntimeIDtoIRObjectMap() const {
    return idToIrMap;
}

bool MapP4RuntimeIdtoIR::preorder(const IR::P4Table *table) {
    if (P4::ControlPlaneAPI::isHidden(table)) {
        return table;
    }
    auto p4RuntimeId = P4::ControlPlaneAPI::getIdAnnotation(table);
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), table);
    } else {
        ::error(
            "Table %1% has no P4Runtime ID associated with it. At this point "
            "every P4 control "
            "plane object should have an ID.");
    }

    return false;
}

bool MapP4RuntimeIdtoIR::preorder(const IR::Type_Header *hdr) {
    if (!P4::ControlPlaneAPI::isControllerHeader(hdr) || P4::ControlPlaneAPI::isHidden(hdr)) {
        return false;
    }
    auto p4RuntimeId = P4::ControlPlaneAPI::getIdAnnotation(hdr);
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), hdr);
    } else {
        ::error(
            "Header %1% has no P4Runtime ID associated with it. At this point "
            "every P4 control "
            "plane object should have an ID.",
            hdr);
    }
    return false;
}

bool MapP4RuntimeIdtoIR::preorder(const IR::P4ValueSet *valueSet) {
    if (P4::ControlPlaneAPI::isHidden(valueSet)) {
        return valueSet;
    }
    auto p4RuntimeId = P4::ControlPlaneAPI::getIdAnnotation(valueSet);
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), valueSet);
    } else {
        ::error(
            "Value set %1% has no P4Runtime ID associated with it. At this point "
            "every P4 "
            "control "
            "plane object should have an ID.",
            valueSet);
    }
    return false;
}

bool MapP4RuntimeIdtoIR::preorder(const IR::P4Action *action) {
    if (P4::ControlPlaneAPI::isHidden(action)) {
        return action;
    }
    auto p4RuntimeId = P4::ControlPlaneAPI::getIdAnnotation(action);
    if (p4RuntimeId.has_value()) {
        idToIrMap.emplace(p4RuntimeId.value(), action);
    } else {
        ::error(
            "P4 Action %1% has no P4Runtime ID associated with it. At this point "
            "every P4 "
            "control "
            "plane object should have an ID.",
            action);
    }
    return false;
}

}  // namespace P4Tools::Flay
