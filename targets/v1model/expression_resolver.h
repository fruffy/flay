#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_EXPRESSION_RESOLVER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/vector.h"

namespace P4Tools::Flay::V1Model {

/// Simplifies an expression, executes method calls, and resolves state references.
class V1ModelExpressionResolver : public ExpressionResolver {
 public:
    explicit V1ModelExpressionResolver(const ProgramInfo &programInfo,
                                       ExecutionState &executionState);

 private:
    const IR::Expression *processTable(const IR::P4Table *table) override;

    const IR::Expression *processExtern(ExternMethodImpls::ExternInfo &externInfo) override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_EXPRESSION_RESOLVER_H_ */
