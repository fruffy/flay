#include "backends/p4tools/modules/flay/targets/tofino/base/expression_resolver.h"

#include <functional>
#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/targets/tofino/base/table_executor.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::Tofino {

TofinoBaseExpressionResolver::TofinoBaseExpressionResolver(const ProgramInfo &programInfo,
                                                           ExecutionState &executionState)
    : ExpressionResolver(programInfo, executionState) {}

const IR::Expression *TofinoBaseExpressionResolver::processTable(const IR::P4Table *table) {
    auto copy = TofinoBaseExpressionResolver(*this);
    auto tableExecutor = TofinoBaseTableExecutor(*table, copy);
    return tableExecutor.processTable();
}

// Provides implementations of Tofino externs.
static const ExternMethodImpls EXTERN_METHOD_IMPLS({});

const IR::Expression *TofinoBaseExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    auto method = EXTERN_METHOD_IMPLS.find(externInfo.externObjectRef, externInfo.methodName,
                                           externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return ExpressionResolver::processExtern(externInfo);
}

}  // namespace P4Tools::Flay::Tofino
