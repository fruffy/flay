#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class SubstitutePlaceHolders : public Transform {
    std::reference_wrapper<const ExecutionState> state;

 public:
    explicit SubstitutePlaceHolders(const ExecutionState &state);

    const IR::Expression *preorder(IR::Placeholder *placeholder) override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_ */
