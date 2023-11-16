#include "backends/p4tools/modules/flay/core/symbolic_executor.h"

#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/node.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

SymbolicExecutor::SymbolicExecutor(const ProgramInfo &programInfo)
    : programInfo(programInfo),
      executionState(programInfo.getProgram()),
      controlPlaneState(FlayTarget::initializeControlPlaneState()) {}

void SymbolicExecutor::run() {
    Util::ScopedTimer timer("Data plane analysis");

    const auto *pipelineSequence = programInfo.getPipelineSequence();
    auto &stepper = FlayTarget::getStepper(programInfo, executionState, controlPlaneState);
    stepper.initializeState();
    for (const auto *node : *pipelineSequence) {
        node->apply(stepper);
    }
}

const ExecutionState &SymbolicExecutor::getExecutionState() { return executionState; }

const ControlPlaneState &SymbolicExecutor::getControlPlaneState() {
    return controlPlaneState.get();
}

}  // namespace P4Tools::Flay
