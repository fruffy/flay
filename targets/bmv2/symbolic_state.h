#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/visitor.h"

namespace P4Tools::Flay::V1Model {

class Bmv2ControlPlaneState : public ControlPlaneState {
 public:
    /// @returns the symbolic boolean variable describing whether a clone session
    /// is active in the program.
    static const IR::SymbolicVariable *getCloneActive();

    /// @returns the symbolic session id variable.
    static const IR::SymbolicVariable *getSessionId(const IR::Type *type);
};

class Bmv2ControlPlaneInitializer : public ControlPlaneStateInitializer {
    Bmv2ControlPlaneState state;

    bool computeMatch(const IR::Expression &entryKey, const IR::SymbolicVariable &keySymbol,
                      cstring tableName, cstring fieldName, cstring matchType,
                      TableKeySet &keySet) override;

 public:
    explicit Bmv2ControlPlaneInitializer(const P4::ReferenceMap &refMap)
        : ControlPlaneStateInitializer(refMap) {}

    std::optional<ControlPlaneConstraints> generateInitialControlPlaneConstraints(
        const IR::P4Program *program) override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_SYMBOLIC_STATE_H_ */
