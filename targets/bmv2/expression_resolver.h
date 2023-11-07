#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_EXPRESSION_RESOLVER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"
#include "ir/ir.h"

namespace P4Tools::Flay::V1Model {

/// Simplifies an expression, executes method calls, and resolves state references.
class V1ModelExpressionResolver : public ExpressionResolver {
 public:
    explicit V1ModelExpressionResolver(const ProgramInfo &programInfo,
                                       ExecutionState &executionState,
                                       ControlPlaneState &controlPlaneState);

 private:
    const IR::Expression *processTable(const IR::P4Table *table) override;

    const IR::Expression *processExtern(const ExternMethodImpls::ExternInfo &externInfo) override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_EXPRESSION_RESOLVER_H_ */
