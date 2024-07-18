#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_STEPPER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/core/interpreter/stepper.h"
#include "backends/p4tools/modules/flay/targets/bmv2/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/bmv2/program_info.h"

namespace P4Tools::Flay::V1Model {

class V1ModelFlayStepper : public FlayStepper {
 protected:
    const Bmv2V1ModelProgramInfo &getProgramInfo() const override;

 public:
    explicit V1ModelFlayStepper(const Bmv2V1ModelProgramInfo &programInfo,
                                ControlPlaneConstraints &constraints,
                                ExecutionState &executionState);

    void initializeState() override;

    void initializeParserState(const IR::P4Parser &parser) override;

    V1ModelExpressionResolver &createExpressionResolver() const override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_STEPPER_H_ */
