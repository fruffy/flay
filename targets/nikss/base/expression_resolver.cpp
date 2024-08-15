#include "backends/p4tools/modules/flay/targets/nikss/base/expression_resolver.h"

#include <functional>
#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/interpreter/externs.h"
#include "backends/p4tools/modules/flay/targets/nikss/base/table_executor.h"
#include "ir/irutils.h"

namespace P4::P4Tools::Flay::Nikss {

NikssBaseExpressionResolver::NikssBaseExpressionResolver(const ProgramInfo &programInfo,
                                                       ControlPlaneConstraints &constraints,
                                                       ExecutionState &executionState)
    : ExpressionResolver(programInfo, constraints, executionState) {}

const IR::Expression *NikssBaseExpressionResolver::processTable(const IR::P4Table *table) {
    return NikssBaseTableExecutor(*table, *this).processTable();
}

// Provides implementations of Nikss externs.
namespace NikssBaseExterns {

const ExternMethodImpls EXTERN_METHOD_IMPLS({});
}  // namespace NikssBaseExterns

const IR::Expression *NikssBaseExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    auto method = NikssBaseExterns::EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return ExpressionResolver::processExtern(externInfo);
}

}  // namespace P4::P4Tools::Flay::Nikss
