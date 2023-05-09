#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_

#include <map>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools {

struct MuxCondComp {
    bool operator()(const IR::Expression *s1, const IR::Expression *s2) const {
        return s1->id < s2->id;
    }
};

class CollapseMux : public Transform {
    std::map<const IR::Expression *, bool, MuxCondComp> conditionMap;

 public:
    CollapseMux() = default;

    explicit CollapseMux(const std::map<const IR::Expression *, bool, MuxCondComp> &conditionMap)
        : conditionMap(conditionMap) {}

    const IR::Node *preorder(IR::Mux *mux) override;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_ */
