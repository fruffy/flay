#include "backends/p4tools/modules/flay/targets/nikss/psa/stepper.h"

namespace P4::P4Tools::Flay::Nikss {

const PsaProgramInfo &PsaFlayStepper::getProgramInfo() const {
    return *NikssBaseFlayStepper::getProgramInfo().checkedTo<PsaProgramInfo>();
}

PsaFlayStepper::PsaFlayStepper(const PsaProgramInfo &programInfo,
                               ControlPlaneConstraints &constraints, ExecutionState &executionState)
    : NikssBaseFlayStepper(programInfo, constraints, executionState) {}

PsaExpressionResolver &PsaFlayStepper::createExpressionResolver() const {
    return *new PsaExpressionResolver(getProgramInfo(), controlPlaneConstraints(),
                                      getExecutionState());
}

}  // namespace P4::P4Tools::Flay::Nikss
