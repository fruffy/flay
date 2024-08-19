#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_PSA_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_PSA_TABLE_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/table_executor.h"
#include "backends/p4tools/modules/flay/targets/nikss/base/table_executor.h"
#include "ir/ir.h"

namespace P4::P4Tools::Flay::Nikss {

/// Executes a table and synthesizes control plane action parameters.
class PsaTableExecutor : public NikssBaseTableExecutor {
 public:
    explicit PsaTableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver);
};

}  // namespace P4::P4Tools::Flay::Nikss

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_PSA_TABLE_EXECUTOR_H_ */
