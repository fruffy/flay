#include "backends/p4tools/modules/flay/targets/v1model/expression_resolver.h"

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

V1ModelExpressionResolver::V1ModelExpressionResolver(const ProgramInfo &programInfo,
                                                     ExecutionState &executionState)
    : ExpressionResolver(programInfo, executionState) {}

const IR::Expression *V1ModelExpressionResolver::processTable(const IR::P4Table *table) {
    auto copy = V1ModelExpressionResolver(*this);
    auto tableExecutor = V1ModelTableExecutor(copy);
    return tableExecutor.processTable(table);
}

}  // namespace P4Tools::Flay
