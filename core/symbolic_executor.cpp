#include "backends/p4tools/modules/flay/core/symbolic_executor.h"

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/core/target.h"

namespace P4Tools::Flay {

SymbolicExecutor::SymbolicExecutor(const ProgramInfo &programInfo)
    : programInfo(programInfo), executionState(programInfo.getProgram()) {}

void SymbolicExecutor::run() {
    const auto *pipelineSequence = programInfo.getPipelineSequence();
    FlayStepper &stepper = FlayTarget::getStepper(programInfo, executionState);
    stepper.initializeState();
    for (const auto *node : *pipelineSequence) {
        node->apply(stepper);
    }
}

}  // namespace P4Tools::Flay
