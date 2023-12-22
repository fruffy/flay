#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_STEPPER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/targets/bmv2/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

namespace P4Tools::Flay::V1Model {

class V1ModelFlayStepper : public FlayStepper {
 protected:
    const Bmv2V1ModelProgramInfo &getProgramInfo() const override;

    Bmv2ControlPlaneState &getControlPlaneState() const override;

 public:
    explicit V1ModelFlayStepper(const Bmv2V1ModelProgramInfo &programInfo,
                                ExecutionState &executionState,
                                Bmv2ControlPlaneState &controlPlaneState);

    void initializeState() override;

    V1ModelExpressionResolver &createExpressionResolver(
        const ProgramInfo &programInfo, ExecutionState &executionState,
        ControlPlaneState &controlPlaneState) const override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_STEPPER_H_ */
