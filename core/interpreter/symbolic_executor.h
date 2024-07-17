#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_SYMBOLIC_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_SYMBOLIC_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"

namespace P4Tools::Flay {

class SymbolicExecutor {
 private:
    /// Target-specific information about the P4 program.
    std::reference_wrapper<const ProgramInfo> _programInfo;

    /// The control plane constraints of the current P4 program.
    /// The data plane might modify some of these.
    std::reference_wrapper<ControlPlaneConstraints> _controlPlaneConstraints;

    /// The current execution state.
    ExecutionState _executionState;

 public:
    virtual ~SymbolicExecutor() = default;
    /// Start running the symbolic executor on the program.
    void run();

    /// Return the execution state associated with this symbolic executor.
    [[nodiscard]] const ExecutionState &executionState() const;

    /// Return the program info associated with this symbolic executor.
    [[nodiscard]] const ProgramInfo &programInfo() const;

    /// Return the control plane constraints associated with this symbolic executor.
    [[nodiscard]] ControlPlaneConstraints &controlPlaneConstraints() const;

    explicit SymbolicExecutor(const ProgramInfo &programInfo,
                              ControlPlaneConstraints &controlPlaneConstraints);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_SYMBOLIC_EXECUTOR_H_ */
