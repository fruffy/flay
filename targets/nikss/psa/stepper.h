#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_PSA_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_PSA_STEPPER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/targets/nikss/base/stepper.h"
#include "backends/p4tools/modules/flay/targets/nikss/psa/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/nikss/psa/program_info.h"

namespace P4::P4Tools::Flay::Nikss {

class PsaFlayStepper : public NikssBaseFlayStepper {
 protected:
    const PsaProgramInfo &getProgramInfo() const override;

 public:
    explicit PsaFlayStepper(const PsaProgramInfo &programInfo, ControlPlaneConstraints &constraints,
                            ExecutionState &executionState);

    PsaExpressionResolver &createExpressionResolver() const override;
};

}  // namespace P4::P4Tools::Flay::Nikss

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_PSA_STEPPER_H_ */
