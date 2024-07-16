#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_BASE_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_BASE_EXPRESSION_RESOLVER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/externs.h"
#include "ir/ir.h"

namespace P4Tools::Flay::Fpga {

/// Simplifies an expression, executes method calls, and resolves state references.
class FpgaBaseExpressionResolver : public ExpressionResolver {
 public:
    explicit FpgaBaseExpressionResolver(const ProgramInfo &programInfo,
                                        ControlPlaneConstraints &constraints,
                                        ExecutionState &executionState);

 protected:
    const IR::Expression *processTable(const IR::P4Table *table) override;

    const IR::Expression *processExtern(const ExternMethodImpls::ExternInfo &externInfo) override;
};

}  // namespace P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_BASE_EXPRESSION_RESOLVER_H_ */
