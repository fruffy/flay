#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_TABLE_EXECUTOR_H_

#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/table_executor.h"
#include "backends/p4tools/modules/flay/targets/fpga/base/table_executor.h"
#include "ir/ir.h"

namespace P4::P4Tools::Flay::Fpga {

/// Executes a table and synthesizes control plane action parameters.
class XsaTableExecutor : public FpgaBaseTableExecutor {
 public:
    explicit XsaTableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver);
};

}  // namespace P4::P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_TABLE_EXECUTOR_H_ */
