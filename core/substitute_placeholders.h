#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class SubstitutePlaceHolders : public Transform {
    class SymbolizePlaceHolders : public Transform {
     public:
        const IR::Expression *postorder(IR::Placeholder *placeholder) override;
    };

    std::reference_wrapper<const ExecutionState> state;
    SymbolizePlaceHolders symbolizer;

 public:
    explicit SubstitutePlaceHolders(const ExecutionState &state);

    const IR::Expression *postorder(IR::Placeholder *placeholder) override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_ */
