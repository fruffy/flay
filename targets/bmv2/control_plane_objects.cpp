#include "backends/p4tools/modules/flay/targets/bmv2/control_plane_objects.h"

#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::V1Model {

bool CloneSession::operator<(const ControlPlaneItem &other) const {
    // There is only one clone session active at a time. We ignore the session id.
    return typeid(*this) == typeid(other) ? false
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

void CloneSession::setSessionId(uint32_t sessionId) { this->sessionId = sessionId; }

const IR::Expression *CloneSession::computeControlPlaneConstraint() const {
    const auto *cloneActive =
        ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
    if (!sessionId.has_value()) {
        return new IR::Equ(cloneActive, IR::BoolLiteral::get(false));
    }
    const auto *sessionIdExpr = IR::Constant::get(IR::Type_Bits::get(32), sessionId.value());
    return new IR::LAnd(new IR::Equ(cloneActive, IR::BoolLiteral::get(false)), sessionIdExpr);
}
}  // namespace P4Tools::Flay::V1Model
