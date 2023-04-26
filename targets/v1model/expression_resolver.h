#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_EXPRESSION_RESOLVER_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/v1model/table_executor.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Simplifies an expression, executes method calls, and resolves state references.
class V1ModelExpressionResolver : public ExpressionResolver {
 public:
    explicit V1ModelExpressionResolver(const ProgramInfo &programInfo,
                                       ExecutionState &executionState);

 private:
    const IR::Expression *processTable(const IR::P4Table *table) override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_EXPRESSION_RESOLVER_H_ */
