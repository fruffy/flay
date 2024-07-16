#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_BASE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_BASE_STEPPER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/targets/fpga/base/program_info.h"

namespace P4Tools::Flay::Fpga {

class FpgaBaseFlayStepper : public FlayStepper {
 protected:
    const FpgaBaseProgramInfo &getProgramInfo() const override;

 public:
    explicit FpgaBaseFlayStepper(const FpgaBaseProgramInfo &programInfo,
                                 ControlPlaneConstraints &constraints,
                                 ExecutionState &executionState);

    void initializeState() override;
};

}  // namespace P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_BASE_STEPPER_H_ */
