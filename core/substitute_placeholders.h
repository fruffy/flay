#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class SubstitutePlaceHolders : public Transform {
    /// A helper visitor which replaces placeholder variables with a symbolic variable with the same
    /// name.
    class SymbolizePlaceHolders : public Transform {
     public:
        const IR::Expression *preorder(IR::Placeholder *placeholder) override;
    };

    /// Reference to an existing execution state.
    std::reference_wrapper<const ExecutionState> state;

    /// The actual symbolizer, which is invoked by the place holder substitution.
    SymbolizePlaceHolders symbolizer;

 public:
    explicit SubstitutePlaceHolders(const ExecutionState &state);

    const IR::Expression *preorder(IR::Placeholder *placeholder) override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTE_PLACEHOLDERS_H_ */
