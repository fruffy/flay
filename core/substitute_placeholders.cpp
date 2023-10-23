#include "backends/p4tools/modules/flay/core/substitute_placeholders.h"

namespace P4Tools::Flay {

const IR::Expression *SubstitutePlaceHolders::preorder(IR::PlaceHolder *placeHolder) {
    const auto *placeHolderVar = new IR::Member(
        placeHolder->type, new IR::PathExpression("*placeholder"), placeHolder->label);
    auto &executionState = state.get();
    if (executionState.exists(placeHolderVar)) {
        auto *expr = executionState.get(placeHolderVar);
        return expr;
    }
    return placeHolder->defaultValue;
}

SubstitutePlaceHolders::SubstitutePlaceHolders(const ExecutionState &state) : state(state) {}

}  // namespace P4Tools::Flay
