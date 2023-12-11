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

class CollapseMux : public Transform {
 private:
    std::map<const IR::Expression *, bool, MuxCondComp> conditionMap;

    explicit CollapseMux(const std::map<const IR::Expression *, bool, MuxCondComp> &conditionMap);

 public:
    CollapseMux() = default;

    const IR::Node *preorder(IR::Mux *mux) override;

    static const IR::Expression *produceOptimizedMux(const IR::Expression *cond,
                                                     const IR::Expression *trueExpression,
                                                     const IR::Expression *falseExpression);

    static const IR::Expression *optimizeExpression(const IR::Expression *expr);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_ */
