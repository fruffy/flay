#include "backends/p4tools/modules/flay/core/substitute_placeholders.h"

namespace P4Tools::Flay {

const IR::Expression *SubstitutePlaceHolders::preorder(IR::Placeholder *placeholder) {
    auto &executionState = state.get();
    auto optValue = executionState.getPlaceholderValue(*placeholder);
    if (optValue.has_value()) {
        return optValue.value();
    }
    return placeholder->defaultValue;
}

SubstitutePlaceHolders::SubstitutePlaceHolders(const ExecutionState &state) : state(state) {}

}  // namespace P4Tools::Flay
