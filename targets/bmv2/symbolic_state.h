#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"
#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/config/v1/p4info.pb.h"
#pragma GCC diagnostic pop

namespace P4Tools::Flay::V1Model {

class Bmv2ControlPlaneState : public ControlPlaneState {
 public:
    /// @returns the symbolic boolean variable describing whether a clone session
    /// is active in the program.
    static const IR::SymbolicVariable *getCloneActive();

    /// @returns the symbolic session id variable.
    static const IR::SymbolicVariable *getSessionId(const IR::Type *type);

    int allocateControlPlaneTable(const IR::P4Table &table) override;

    /// Initializes a control plane session default assignment and returns the
    /// appropriate variable.
    const IR::SymbolicVariable *allocateCloneSession();
};

class ProtobufBmv2ControlPlaneState : public Bmv2ControlPlaneState {
 public:
    int initializeDefaultState(const p4::config::v1::P4Info &p4info,
                               const P4RuntimeIdtoIrNodeMap &idMapper,
                               const P4::ReferenceMap &refMap);
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_ */
