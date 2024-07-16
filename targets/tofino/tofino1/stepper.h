#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO1_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO1_STEPPER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/targets/tofino/base/stepper.h"
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/program_info.h"

namespace P4Tools::Flay::Tofino {

class Tofino1FlayStepper : public TofinoBaseFlayStepper {
 protected:
    const Tofino1ProgramInfo &getProgramInfo() const override;

 public:
    explicit Tofino1FlayStepper(const Tofino1ProgramInfo &programInfo,
                                ControlPlaneConstraints &constraints,
                                ExecutionState &executionState);

    void initializeParserState(const IR::P4Parser &parser) override;

    Tofino1ExpressionResolver &createExpressionResolver() const override;
};

}  // namespace P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO1_STEPPER_H_ */
