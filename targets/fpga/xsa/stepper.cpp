#include "backends/p4tools/modules/flay/targets/fpga/xsa/stepper.h"

#include <utility>

#include "backends/p4tools/modules/flay/core/program_info.h"

namespace P4Tools::Flay::Fpga {

const XsaProgramInfo &XsaFlayStepper::getProgramInfo() const {
    return *FpgaBaseFlayStepper::getProgramInfo().checkedTo<XsaProgramInfo>();
}

XsaFlayStepper::XsaFlayStepper(const XsaProgramInfo &programInfo, ExecutionState &executionState)
    : FpgaBaseFlayStepper(programInfo, executionState) {}

XsaExpressionResolver &XsaFlayStepper::createExpressionResolver(
    const ProgramInfo &programInfo, ExecutionState &executionState) const {
    return *new XsaExpressionResolver(programInfo, executionState);
}

}  // namespace P4Tools::Flay::Fpga
