#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_SPECIALIZER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_SPECIALIZER_H_

#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

namespace P4Tools::Flay {

/// Specialize the Program
class FlaySpecializer : public PassManager {
    ElimDeadCode *_elimDeadCode = nullptr;

 public:
    explicit FlaySpecializer(P4::ReferenceMap &refMap,
                             const AbstractReachabilityMap &reachabilityMap)
        : _elimDeadCode(new ElimDeadCode(refMap, reachabilityMap)) {
        addPasses({
            _elimDeadCode,
        });
    }

    [[nodiscard]] std::vector<EliminatedReplacedPair> eliminatedNodes() const {
        return _elimDeadCode->eliminatedNodes();
    }
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_SPECIALIZER_H_ */
