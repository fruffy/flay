#include "backends/p4tools/modules/flay/targets/bmv2/control_plane_objects.h"

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::V1Model {

bool CloneSession::operator<(const ControlPlaneItem &other) const {
    // There is only one clone session active at a time. We ignore the session id.
    return typeid(*this) == typeid(other) ? false
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

void CloneSession::setSessionId(uint32_t sessionId) { this->sessionId = sessionId; }

ControlPlaneAssignmentSet CloneSession::computeControlPlaneAssignments() const {
    ControlPlaneAssignmentSet assignmentSet;
    const auto *cloneActive = ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(),
                                                                  cstring("clone_session_active"));
    if (!sessionId.has_value()) {
        assignmentSet.emplace(*cloneActive, *IR::BoolLiteral::get(false));
    } else {
        assignmentSet.emplace(*cloneActive, *IR::BoolLiteral::get(true));
        assignmentSet.emplace(*Bmv2ControlPlaneState::getCloneSessionId(IR::Type_Bits::get(32)),
                              *IR::Constant::get(IR::Type_Bits::get(32), sessionId.value()));
    }
    return assignmentSet;
}

Z3ControlPlaneAssignmentSet CloneSession::computeZ3ControlPlaneAssignments() const {
    Z3ControlPlaneAssignmentSet assignmentSet;
    const auto *cloneActive = ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(),
                                                                  cstring("clone_session_active"));
    if (!sessionId.has_value()) {
        assignmentSet.add(*cloneActive, Z3Cache::set(IR::BoolLiteral::get(false)));
    } else {
        assignmentSet.add(*cloneActive, Z3Cache::set(IR::BoolLiteral::get(true)));
        assignmentSet.add(
            *Bmv2ControlPlaneState::getCloneSessionId(IR::Type_Bits::get(32)),
            Z3Cache::set(IR::Constant::get(IR::Type_Bits::get(32), sessionId.value())));
    }
    return assignmentSet;
}

}  // namespace P4Tools::Flay::V1Model
