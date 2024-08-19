#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_EXPRESSION_RESOLVER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/externs.h"
#include "ir/ir.h"

namespace P4::P4Tools::Flay::Nikss {

/// Simplifies an expression, executes method calls, and resolves state references.
class NikssBaseExpressionResolver : public ExpressionResolver {
 public:
    explicit NikssBaseExpressionResolver(const ProgramInfo &programInfo,
                                         ControlPlaneConstraints &constraints,
                                         ExecutionState &executionState);

 protected:
    const IR::Expression *processTable(const IR::P4Table *table) override;

    const IR::Expression *processExtern(const ExternMethodImpls::ExternInfo &externInfo) override;
};

}  // namespace P4::P4Tools::Flay::Nikss

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_EXPRESSION_RESOLVER_H_ */
