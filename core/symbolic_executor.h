#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SYMBOLIC_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SYMBOLIC_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"

namespace P4Tools::Flay {

class SymbolicExecutor {
 private:
    /// Target-specific information about the P4 program.
    const ProgramInfo &programInfo;

    /// The current execution state.
    ExecutionState executionState;

 protected:
 public:
    virtual ~SymbolicExecutor() = default;

    SymbolicExecutor(const SymbolicExecutor &) = delete;

    SymbolicExecutor(SymbolicExecutor &&) = delete;

    SymbolicExecutor &operator=(const SymbolicExecutor &) = delete;

    SymbolicExecutor &operator=(SymbolicExecutor &&) = delete;

    void run();

    explicit SymbolicExecutor(const ProgramInfo &programInfo);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SYMBOLIC_EXECUTOR_H_ */
