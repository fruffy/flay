
#include "backends/p4tools/modules/flay/targets/v1model/table_executor.h"

#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

V1ModelTableExecutor::V1ModelTableExecutor(ExpressionResolver &callingResolver)
    : TableExecutor(callingResolver) {}

}  // namespace P4Tools::Flay
