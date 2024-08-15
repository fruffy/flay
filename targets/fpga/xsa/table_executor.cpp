
#include "backends/p4tools/modules/flay/targets/fpga/xsa/table_executor.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/fpga/constants.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4::P4Tools::Flay::Fpga {

XsaTableExecutor::XsaTableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver)
    : FpgaBaseTableExecutor(table, callingResolver) {}

}  // namespace P4::P4Tools::Flay::Fpga
