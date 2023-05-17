#include "backends/p4tools/modules/flay/core/symbolic_executor.h"

#include <vector>

#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/node.h"

namespace P4Tools::Flay {

SymbolicExecutor::SymbolicExecutor(const ProgramInfo &programInfo)
    : programInfo(programInfo), executionState(programInfo.getProgram()) {}

void SymbolicExecutor::run() {
    const auto *pipelineSequence = programInfo.getPipelineSequence();
    auto &stepper = FlayTarget::getStepper(programInfo, executionState);
    stepper.initializeState();
    for (const auto *node : *pipelineSequence) {
        node->apply(stepper);
    }
    executionState.printSymbolicEnv();
}

}  // namespace P4Tools::Flay
