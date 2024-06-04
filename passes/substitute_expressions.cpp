#include "backends/p4tools/modules/flay/passes/substitute_expressions.h"

#include "ir/node.h"

namespace P4Tools::Flay {

SubstituteExpressions::SubstituteExpressions(const P4::ReferenceMap &refMap,
                                             const AbstractReachabilityMap &reachabilityMap)
    : reachabilityMap(reachabilityMap), refMap(refMap) {}

const IR::Node *SubstituteExpressions::preorder(IR::PathExpression *pathExpression) {
    return pathExpression;
}

const IR::Node *SubstituteExpressions::preorder(IR::Member *member) { return member; }

std::vector<EliminatedReplacedPair> SubstituteExpressions::eliminatedNodes() const {
    return _eliminatedNodes;
}

}  // namespace P4Tools::Flay
