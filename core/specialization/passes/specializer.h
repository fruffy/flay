#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SPECIALIZER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SPECIALIZER_H_

#include "backends/p4tools/modules/flay/core/specialization/passes//substitute_expressions.h"
#include "backends/p4tools/modules/flay/core/specialization/passes/elim_dead_code.h"

namespace P4Tools::Flay {

/// The eliminated and optionally replaced node.
using EliminatedReplacedPair = std::pair<const IR::Node *, const IR::Node *>;

/// Specialize the Program
class FlaySpecializer : public PassManager {
    ElimDeadCode *_elimDeadCode = nullptr;

    SubstituteExpressions *_substituteExpressions = nullptr;

 public:
    explicit FlaySpecializer(const P4::ReferenceMap &refMap,
                             const AbstractReachabilityMap &reachabilityMap,
                             const AbstractSubstitutionMap &substitutionMap)
        : _elimDeadCode(new ElimDeadCode(refMap, reachabilityMap)),
          _substituteExpressions(new SubstituteExpressions(refMap, substitutionMap)) {
        addPasses({
            _elimDeadCode,
            _substituteExpressions,
        });
    }

    [[nodiscard]] std::vector<EliminatedReplacedPair> eliminatedNodes() const {
        std::vector<EliminatedReplacedPair> result;
        auto deadNodes = _elimDeadCode->eliminatedNodes();
        auto substitutedNodes = _substituteExpressions->eliminatedNodes();
        result.insert(result.end(), deadNodes.begin(), deadNodes.end());
        result.insert(result.end(), substitutedNodes.begin(), substitutedNodes.end());
        return result;
    }
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SPECIALIZER_H_ */
