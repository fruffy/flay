#include "backends/p4tools/modules/flay/core/symbolic_executor.h"

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/node.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

SymbolicExecutor::SymbolicExecutor(const ProgramInfo &programInfo)
    : programInfo(programInfo), executionState(&programInfo.getP4Program()) {}

void SymbolicExecutor::run() {
    Util::ScopedTimer timer("Data plane analysis");

    const auto *pipelineSequence = programInfo.getPipelineSequence();
    auto &stepper = FlayTarget::getStepper(programInfo, executionState);
    stepper.initializeState();
    for (const auto *node : *pipelineSequence) {
        node->apply(stepper);
    }
    // executionState.printSymbolicEnv();
    // exit(1);
    /// Substitute any placeholder variables encountered in the execution state.
    printInfo("Substituting placeholder variables...");
    executionState.substitutePlaceholders();
}

const ExecutionState &SymbolicExecutor::getExecutionState() { return executionState; }

}  // namespace P4Tools::Flay
