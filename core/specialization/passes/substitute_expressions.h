#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SUBSTITUTE_EXPRESSIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SUBSTITUTE_EXPRESSIONS_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/specialization/passes/specialization_statistics.h"
#include "backends/p4tools/modules/flay/core/specialization/substitution_map.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// This compiler pass looks up program nodes in the expression map and substitutes any node in the
/// map can be replaced with a constant.
class SubstituteExpressions : public Transform {
    /// The reachability map computed by the execution state.
    std::reference_wrapper<const AbstractSubstitutionMap> _substitutionMap;

    std::reference_wrapper<const P4::ReferenceMap> _refMap;

    /// The list of eliminated and optionally replaced nodes. Used for bookkeeping.
    std::vector<EliminatedReplacedPair> _eliminatedNodes;

    const IR::Node *preorder(IR::Member *member) override;
    const IR::Node *preorder(IR::AssignmentStatement *statement) override;
    const IR::Node *preorder(IR::Declaration_Variable *declaration) override;
    const IR::Node *preorder(IR::MethodCallExpression *call) override;
    const IR::Node *preorder(IR::PathExpression *pathExpression) override;

 public:
    SubstituteExpressions() = delete;

    explicit SubstituteExpressions(const P4::ReferenceMap &refMap,
                                   const AbstractSubstitutionMap &substitutionMap);

    [[nodiscard]] std::vector<EliminatedReplacedPair> eliminatedNodes() const;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SPECIALIZATION_PASSES_SUBSTITUTE_EXPRESSIONS_H_ */
