#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/targets/bmv2/control_plane_objects.h"

namespace P4Tools::Flay::V1Model {

const IR::SymbolicVariable *Bmv2ControlPlaneState::getCloneActive() {
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
}

const IR::SymbolicVariable *Bmv2ControlPlaneState::getSessionId(const IR::Type *type) {
    return ToolsVariables::getSymbolicVariable(type, "clone_session_id");
}

std::optional<ControlPlaneConstraints>
Bmv2ControlPlaneInitializer::generateInitialControlPlaneConstraints(const IR::P4Program *program) {
    defaultConstraints.emplace("clone_session", *new CloneSession(std::nullopt));

    program->apply(*this);
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    return getDefaultConstraints();
}
}  // namespace P4Tools::Flay::V1Model
