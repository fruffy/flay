#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_SUBSTITUTE_VARIABLE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_SUBSTITUTE_VARIABLE_H_

// TODO: We should not depend on execution state here.
#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4::P4Tools::Flay {

class SubstituteSymbolicVariable : public Transform {
    /// Reference to an existing execution state.
    const ControlPlaneAssignmentSet &symbolicVariables;

 public:
    explicit SubstituteSymbolicVariable(const ControlPlaneAssignmentSet &symbolicVariables);

    const IR::Expression *preorder(IR::SymbolicVariable *placeholder) override;
};

}  // namespace P4::P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_SUBSTITUTE_VARIABLE_H_ */
