#include "backends/p4tools/modules/flay/core/interpreter/substitute_placeholders.h"

#include "lib/map.h"

namespace P4Tools::Flay {

const IR::Expression *SubstitutePlaceHolders::SymbolizePlaceHolders::preorder(
    IR::Placeholder *placeholder) {
    return new IR::SymbolicVariable(placeholder->type, "*placeholder" + placeholder->label);
}

const IR::Expression *SubstitutePlaceHolders::preorder(IR::Placeholder *placeholder) {
    const auto &executionState = state.get();
    auto optValue = executionState.getPlaceholderValue(*placeholder);
    if (optValue.has_value()) {
        /// Resolve any placeholders that are part of the return value as symbolic variables.
        const auto *value = optValue.value();
        return value->apply(symbolizer);
    }
    return placeholder->defaultValue;
}

SubstitutePlaceHolders::SubstitutePlaceHolders(const ExecutionState &state) : state(state) {
    setName("SubstitutePlaceHolders");
    visitDagOnce = false;
}

}  // namespace P4Tools::Flay
