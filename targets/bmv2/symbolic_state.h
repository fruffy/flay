#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"

namespace P4Tools::Flay::V1Model {

class Bmv2ControlPlaneState : public ControlPlaneState {
 public:
    /// @returns the symbolic boolean variable describing whether a clone session is active in the
    /// program.
    static const IR::SymbolicVariable *getCloneActive();

    /// @returns the symbolic session id variable.
    static const IR::SymbolicVariable *getSessionId(const IR::Type *type);

    const IR::SymbolicVariable *allocateControlPlaneTable(cstring tableName) override;

    /// Initializes a control plane session default assignment and returns the appropriate variable.
    const IR::SymbolicVariable *allocateCloneSession();
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_ */
