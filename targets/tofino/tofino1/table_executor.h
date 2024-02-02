#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO1_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO1_TABLE_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"
#include "backends/p4tools/modules/flay/targets/tofino/base/table_executor.h"
#include "ir/ir.h"

namespace P4Tools::Flay::Tofino {

/// Executes a table and synthesizes control plane action parameters.
class Tofino1TableExecutor : public TofinoBaseTableExecutor {
 public:
    explicit Tofino1TableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver);
};

}  // namespace P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO1_TABLE_EXECUTOR_H_ */
