#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_

#include <functional>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class ElimDeadCode : public Transform {
    /// The computed execution state.
    std::reference_wrapper<const ExecutionState> executionState;

    /// The constraint solver associated with the tool. Currently specialized to Z3.
    std::reference_wrapper<AbstractSolver> solver;

    /// This flags records whether this class has eliminated dead code.
    bool deletedCode = false;

    /// The set of active control plane constraints. These constraints are added to every solver
    /// check to compute feasibility of a program node.
    ControlPlaneConstraints controlPlaneConstraints;

 public:
    ElimDeadCode() = delete;

    explicit ElimDeadCode(const ExecutionState &executionState, AbstractSolver &solver);

    const IR::Node *postorder(IR::IfStatement *stmt) override;
    void end_apply() override;

    /// Add control plane constraints to the tool. These constraints are added to every
    /// solver check.
    void addControlPlaneConstraints(const ControlPlaneConstraints &newControlPlaneConstraints);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_ */
