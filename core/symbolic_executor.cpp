#include "backends/p4tools/modules/flay/core/symbolic_executor.h"

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/node.h"
#include "lib/timer.h"

namespace P4Tools::Flay {

SymbolicExecutor::SymbolicExecutor(const ProgramInfo &programInfo,
                                   ControlPlaneConstraints &controlPlaneConstraints)
    : _programInfo(programInfo),
      _controlPlaneConstraints(controlPlaneConstraints),
      _executionState(&programInfo.getP4Program()) {}

void SymbolicExecutor::run() {
    Util::ScopedTimer timer("Data plane analysis");

    const auto *pipelineSequence = _programInfo.get().getPipelineSequence();
    auto &stepper = FlayTarget::getStepper(_programInfo, _controlPlaneConstraints, _executionState);
    stepper.initializeState();
    for (const auto *node : *pipelineSequence) {
        node->apply(stepper);
    }
    /// Substitute any placeholder variables encountered in the execution state.
    printInfo("Substituting placeholder variables...");
    _executionState.substitutePlaceholders();
}

const ExecutionState &SymbolicExecutor::executionState() const { return _executionState; }

ControlPlaneConstraints &SymbolicExecutor::controlPlaneConstraints() const {
    return _controlPlaneConstraints;
}

const ProgramInfo &SymbolicExecutor::programInfo() const { return _programInfo; }

}  // namespace P4Tools::Flay
