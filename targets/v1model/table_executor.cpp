
#include "backends/p4tools/modules/flay/targets/v1model/table_executor.h"

#include "backends/p4tools/modules/flay/core/expression_resolver.h"

namespace P4Tools::Flay {

V1ModelTableExecutor::V1ModelTableExecutor(ExpressionResolver &callingResolver)
    : TableExecutor(callingResolver) {}

}  // namespace P4Tools::Flay
