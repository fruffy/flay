#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_TABLE_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/table_executor.h"
#include "ir/ir.h"

namespace P4::P4Tools::Flay::Tofino {

/// Executes a table and synthesizes control plane action parameters.
class TofinoBaseTableExecutor : public TableExecutor {
 protected:
    /// Compute a table key match. Can be overridden by targets to add more match types.
    /// @returns the full match expression for the key given the table match types.
    const TableMatchKey *computeTargetMatchType(const IR::KeyElement *keyField) const override;

 public:
    explicit TofinoBaseTableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver);
};

}  // namespace P4::P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_BASE_TABLE_EXECUTOR_H_ */
