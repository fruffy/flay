#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TABLE_EXECUTOR_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Executes a table and synthesizes control plane action parameters.
class V1ModelTableExecutor : public TableExecutor {
 public:
    explicit V1ModelTableExecutor(ExpressionResolver &callingResolver);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TABLE_EXECUTOR_H_ */
