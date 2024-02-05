#include "backends/p4tools/modules/flay/targets/tofino/tofino1/stepper.h"

#include <utility>

#include "backends/p4tools/modules/flay/core/program_info.h"

namespace P4Tools::Flay::Tofino {

const Tofino1ProgramInfo &Tofino1FlayStepper::getProgramInfo() const {
    return *TofinoBaseFlayStepper::getProgramInfo().checkedTo<Tofino1ProgramInfo>();
}

Tofino1FlayStepper::Tofino1FlayStepper(const Tofino1ProgramInfo &programInfo,
                                       ExecutionState &executionState)
    : TofinoBaseFlayStepper(programInfo, executionState) {}

Tofino1ExpressionResolver &Tofino1FlayStepper::createExpressionResolver(
    const ProgramInfo &programInfo, ExecutionState &executionState) const {
    return *new Tofino1ExpressionResolver(programInfo, executionState);
}

}  // namespace P4Tools::Flay::Tofino
