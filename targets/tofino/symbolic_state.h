#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/visitor.h"

namespace P4Tools::Flay::Tofino {

class TofinoControlPlaneInitializer : public ControlPlaneStateInitializer {
    bool computeMatch(const IR::Expression &entryKey, const IR::SymbolicVariable &keySymbol,
                      cstring tableName, cstring fieldName, cstring matchType,
                      TableKeySet &keySet) override;

 public:
    explicit TofinoControlPlaneInitializer(const P4::ReferenceMap &refMap)
        : ControlPlaneStateInitializer(refMap) {}

    std::optional<ControlPlaneConstraints> generateInitialControlPlaneConstraints(
        const IR::P4Program *program) override;
};

}  // namespace P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_SYMBOLIC_STATE_H_ */
