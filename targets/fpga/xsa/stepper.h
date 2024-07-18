#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_STEPPER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/targets/fpga/base/stepper.h"
#include "backends/p4tools/modules/flay/targets/fpga/xsa/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/fpga/xsa/program_info.h"

namespace P4Tools::Flay::Fpga {

class XsaFlayStepper : public FpgaBaseFlayStepper {
 protected:
    const XsaProgramInfo &getProgramInfo() const override;

 public:
    explicit XsaFlayStepper(const XsaProgramInfo &programInfo, ControlPlaneConstraints &constraints,
                            ExecutionState &executionState);

    void initializeParserState(const IR::P4Parser &parser) override;

    XsaExpressionResolver &createExpressionResolver() const override;
};

}  // namespace P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_STEPPER_H_ */
