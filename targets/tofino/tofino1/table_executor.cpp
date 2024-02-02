
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/table_executor.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/tofino/constants.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay::Tofino {

Tofino1TableExecutor::Tofino1TableExecutor(const IR::P4Table &table,
                                           ExpressionResolver &callingResolver)
    : TofinoBaseTableExecutor(table, callingResolver) {}

}  // namespace P4Tools::Flay::Tofino
