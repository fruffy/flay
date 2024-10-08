#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_STEPPER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/stepper.h"
#include "backends/p4tools/modules/flay/targets/tofino/base/program_info.h"

namespace P4::P4Tools::Flay::Tofino {

class TofinoBaseFlayStepper : public FlayStepper {
 protected:
    const TofinoBaseProgramInfo &getProgramInfo() const override;

 public:
    explicit TofinoBaseFlayStepper(const TofinoBaseProgramInfo &programInfo,
                                   ControlPlaneConstraints &constraints,
                                   ExecutionState &executionState);

    void initializeState() override;
};

}  // namespace P4::P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_STEPPER_H_ */
