#include "backends/p4tools/modules/flay/core/collapse_mux.h"

#include "lib/log.h"

namespace P4Tools {

const IR::Node *CollapseMux::preorder(IR::Mux *mux) {
    const auto *cond = mux->e0;
    auto condIt = conditionMap.find(cond);
    if (condIt != conditionMap.end()) {
        if (condIt->second) {
            return mux->e1;
        }
        return mux->e2;
    }
    auto conditionMapE1 = conditionMap;
    conditionMapE1.emplace(cond, true);
    const auto *e1 = mux->e1->apply(CollapseMux(conditionMapE1));
    mux->e1 = e1;
    auto conditionMapE2 = conditionMap;
    conditionMapE2.emplace(cond, false);
    const auto *e2 = mux->e2->apply(CollapseMux(conditionMapE2));
    mux->e2 = e2;
    return mux;
}

}  // namespace P4Tools
