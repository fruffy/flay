#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TABLE_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"

namespace P4Tools::Flay::V1Model {

/// Executes a table and synthesizes control plane action parameters.
class V1ModelTableExecutor : public TableExecutor {
 protected:
    /// Compute a table key match. Can be overridden by targets to add more match types.
    /// @returns the full match expression for the key given the table match types.
    const IR::Expression *computeTargetMatchType(const IR::KeyElement *keyField) const override;

 public:
    explicit V1ModelTableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver);
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TABLE_EXECUTOR_H_ */
