#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_

#include <map>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools {

struct MuxCondComp {
    bool operator()(const IR::Expression *s1, const IR::Expression *s2) const;
};

using ConditionMap = std::map<const IR::Expression *, bool, MuxCondComp>;

class CollapseExpression : public Transform {
 private:
    ConditionMap conditionMap;

 public:
    explicit CollapseExpression(
        const std::map<const IR::Expression *, bool, MuxCondComp> &conditionMap)
        : conditionMap(conditionMap) {}
    CollapseExpression() = default;

    const IR::Node *preorder(IR::Expression *expr) override;
    const IR::Node *preorder(IR::LAnd *expr) override;
    const IR::Node *preorder(IR::LOr *expr) override;
    const IR::Node *preorder(IR::LNot *expr) override;
};

class CollapseMux : public Transform {
 private:
    ConditionMap conditionMap;

 public:
    explicit CollapseMux(const std::map<const IR::Expression *, bool, MuxCondComp> &conditionMap);
    CollapseMux() = default;

    const IR::Node *preorder(IR::Mux *mux) override;
    // const IR::Node *preorder(IR::Expression *expr) override;
    // const IR::Node *preorder(IR::LAnd *expr) override;
    // const IR::Node *preorder(IR::LOr *expr) override;
    // const IR::Node *preorder(IR::LNot *expr) override;

    /// Produce a Mux expression where the amount of sub-expressions has been minimized.
    static const IR::Expression *produceOptimizedMux(const IR::Expression *cond,
                                                     const IR::Expression *trueExpression,
                                                     const IR::Expression *falseExpression);

    static const IR::Expression *optimizeExpression(const IR::Expression *expr);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_ */
