#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_STEPPER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/stepper.h"
#include "backends/p4tools/modules/flay/targets/nikss/base/program_info.h"

namespace P4::P4Tools::Flay::Nikss {

class NikssBaseFlayStepper : public FlayStepper {
 protected:
    const NikssBaseProgramInfo &getProgramInfo() const override;

 public:
    explicit NikssBaseFlayStepper(const NikssBaseProgramInfo &programInfo,
                                 ControlPlaneConstraints &constraints,
                                 ExecutionState &executionState);

    void initializeState() override;
};

}  // namespace P4::P4Tools::Flay::Nikss

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_STEPPER_H_ */
