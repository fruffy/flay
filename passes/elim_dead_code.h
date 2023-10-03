#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_

#include <functional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class ElimDeadCode : public Transform {
    std::reference_wrapper<const ExecutionState> executionState;

    Z3Solver solver;

    bool deletedCode = false;

 public:
    ElimDeadCode() = delete;

    explicit ElimDeadCode(const ExecutionState &executionState);

    const IR::Node *postorder(IR::IfStatement *stmt) override;

    void end_apply() override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_ */
