#include "backends/p4tools/modules/flay/passes/substitute_expressions.h"

#include <optional>

#include "ir/node.h"

namespace P4Tools::Flay {

SubstituteExpressions::SubstituteExpressions(const P4::ReferenceMap &refMap,
                                             const AbstractSubstitutionMap &substitutionMap)
    : _substitutionMap(substitutionMap), _refMap(refMap) {}

const IR::Node *SubstituteExpressions::preorder(IR::Declaration_Variable *declaration) {
    if (declaration->initializer != nullptr) {
        declaration->initializer =
            declaration->initializer->apply(SubstituteExpressions(_refMap, _substitutionMap));
    }
    return declaration;
}

const IR::Node *SubstituteExpressions::preorder(IR::AssignmentStatement *statement) {
    // Only analyze the right hand side of the assignment.
    prune();
    statement->right = statement->right->apply(SubstituteExpressions(_refMap, _substitutionMap));
    return statement;
}

const IR::Node *SubstituteExpressions::preorder(IR::PathExpression *pathExpression) {
    if (!pathExpression->getSourceInfo().isValid() || !pathExpression->type->is<IR::Type_Bits>()) {
        return pathExpression;
    }
    auto optConstant = _substitutionMap.get().isExpressionConstant(pathExpression);
    if (optConstant.has_value()) {
        return optConstant.value();
    }
    return pathExpression;
}

const IR::Node *SubstituteExpressions::preorder(IR::Member *member) {
    if (!member->getSourceInfo().isValid() || !member->type->is<IR::Type_Bits>()) {
        return member;
    }

    auto optConstant = _substitutionMap.get().isExpressionConstant(member);
    if (optConstant.has_value()) {
        return optConstant.value();
    }
    return member;
}

std::vector<EliminatedReplacedPair> SubstituteExpressions::eliminatedNodes() const {
    return _eliminatedNodes;
}

}  // namespace P4Tools::Flay
