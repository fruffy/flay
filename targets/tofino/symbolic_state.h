#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/core/control_plane/symbolic_state.h"

namespace P4Tools::Flay::Tofino {

class TofinoControlPlaneInitializer : public ControlPlaneStateInitializer {
    bool computeMatch(const IR::Expression &entryKey, const IR::SymbolicVariable &keySymbol,
                      cstring tableName, cstring fieldName, cstring matchType,
                      ControlPlaneAssignmentSet &keySet) override;

    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Declaration_Instance *declaration) override;

    /// Check whether the table has an action-profile implementation and return the name of the
    /// action profile if so.
    std::optional<cstring> associateActionProfiles(
        const IR::P4Table &table, ControlPlaneConstraints &_defaultConstraints) const;

 public:
    TofinoControlPlaneInitializer() = default;

    std::optional<ControlPlaneConstraints> generateInitialControlPlaneConstraints(
        const IR::P4Program *program) override;
};

}  // namespace P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_SYMBOLIC_STATE_H_ */
