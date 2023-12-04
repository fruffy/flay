#include "backends/p4tools/modules/flay/core/collapse_mux.h"

#include <utility>

namespace P4Tools {

bool MuxCondComp::operator()(const IR::Expression *s1, const IR::Expression *s2) const {
    return s1->clone_id < s2->clone_id;
}

const IR::Node *CollapseMux::preorder(IR::Mux *mux) {
    prune();
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

const IR::Expression *CollapseMux::produceOptimizedMux(const IR::Expression *cond,
                                                       const IR::Expression *trueExpression,
                                                       const IR::Expression *falseExpression) {
    auto *mux = new IR::Mux(trueExpression->type, cond, trueExpression, falseExpression);
    return mux->apply(CollapseMux());
}

CollapseMux::CollapseMux(const std::map<const IR::Expression *, bool, MuxCondComp> &conditionMap)
    : conditionMap(conditionMap) {}

}  // namespace P4Tools
