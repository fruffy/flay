#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_EXPRESSION_RESOLVER_H_

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/externs.h"
#include "ir/ir.h"

namespace P4::P4Tools::Flay::Tofino {

/// Simplifies an expression, executes method calls, and resolves state references.
class TofinoBaseExpressionResolver : public ExpressionResolver {
 public:
    explicit TofinoBaseExpressionResolver(const ProgramInfo &programInfo,
                                          ControlPlaneConstraints &constraints,
                                          ExecutionState &executionState);

 protected:
    const IR::Expression *processTable(const IR::P4Table *table) override;

    const IR::Expression *processExtern(const ExternMethodImpls::ExternInfo &externInfo) override;
};

}  // namespace P4::P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_EXPRESSION_RESOLVER_H_ */
