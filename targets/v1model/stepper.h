#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_STEPPER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/targets/v1model/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/v1model/program_info.h"

namespace P4Tools::Flay::V1Model {

class V1ModelFlayStepper : public FlayStepper {
 protected:
    const V1ModelProgramInfo &getProgramInfo() const override;

 public:
    explicit V1ModelFlayStepper(const V1ModelProgramInfo &programInfo,
                                ExecutionState &executionState);

    void initializeState() override;

    V1ModelExpressionResolver &createExpressionResolver(
        const ProgramInfo &programInfo, ExecutionState &executionState) const override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_STEPPER_H_ */
