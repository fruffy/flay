#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_SUBSTITUTE_EXPRESSIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_SUBSTITUTE_EXPRESSIONS_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/reachability.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// The eliminated and optionally replaced node.
using EliminatedReplacedPair = std::pair<const IR::Node *, const IR::Node *>;

/// This compiler pass looks up program nodes in the reachability map and deletes nodes which are
/// not executable according to the computation in the map.
class SubstituteExpressions : public Transform {
    /// The reachability map computed by the execution state.
    std::reference_wrapper<const AbstractReachabilityMap> reachabilityMap;

    std::reference_wrapper<const P4::ReferenceMap> refMap;

    /// The list of eliminated and optionally replaced nodes. Used for bookkeeping.
    std::vector<EliminatedReplacedPair> _eliminatedNodes;

    const IR::Node *preorder(IR::PathExpression *pathExpression) override;
    const IR::Node *preorder(IR::Member *member) override;

 public:
    SubstituteExpressions() = delete;

    explicit SubstituteExpressions(const P4::ReferenceMap &refMap,
                                   const AbstractReachabilityMap &reachabilityMap);

    [[nodiscard]] std::vector<EliminatedReplacedPair> eliminatedNodes() const;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_SUBSTITUTE_EXPRESSIONS_H_ */
