#include "backends/p4tools/modules/flay/targets/fpga/base/expression_resolver.h"

#include <functional>
#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/targets/fpga/base/table_executor.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::Fpga {

FpgaBaseExpressionResolver::FpgaBaseExpressionResolver(const ProgramInfo &programInfo,
                                                       ExecutionState &executionState)
    : ExpressionResolver(programInfo, executionState) {}

const IR::Expression *FpgaBaseExpressionResolver::processTable(const IR::P4Table *table) {
    auto copy = FpgaBaseExpressionResolver(*this);
    auto tableExecutor = FpgaBaseTableExecutor(*table, copy);
    return tableExecutor.processTable();
}

// Provides implementations of Fpga externs.
namespace FpgaBaseExterns {

const ExternMethodImpls EXTERN_METHOD_IMPLS({});
}  // namespace FpgaBaseExterns

const IR::Expression *FpgaBaseExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    auto method = FpgaBaseExterns::EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return ExpressionResolver::processExtern(externInfo);
}

}  // namespace P4Tools::Flay::Fpga
