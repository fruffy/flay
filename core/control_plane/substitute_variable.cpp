#include "backends/p4tools/modules/flay/core/control_plane/substitute_variable.h"

#include "lib/map.h"

namespace P4::P4Tools::Flay {

const IR::Expression *SubstituteSymbolicVariable::preorder(IR::SymbolicVariable *placeholder) {
    auto value = symbolicVariables.find(*placeholder);
    if (value != symbolicVariables.end()) {
        return &value->second.get();
    }
    return placeholder;
}

SubstituteSymbolicVariable::SubstituteSymbolicVariable(
    const ControlPlaneAssignmentSet &symbolicVariables)
    : symbolicVariables(symbolicVariables) {
    setName("SubstitutePlaceHolders");
    visitDagOnce = false;
}

}  // namespace P4::P4Tools::Flay
