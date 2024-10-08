#include "backends/p4tools/modules/flay/targets/tofino/tofino1/expression_resolver.h"

#include <functional>
#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/interpreter/externs.h"
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/table_executor.h"
#include "ir/irutils.h"

namespace P4::P4Tools::Flay::Tofino {

Tofino1ExpressionResolver::Tofino1ExpressionResolver(const ProgramInfo &programInfo,
                                                     ControlPlaneConstraints &constraints,
                                                     ExecutionState &executionState)
    : TofinoBaseExpressionResolver(programInfo, constraints, executionState) {}

const IR::Expression *Tofino1ExpressionResolver::processTable(const IR::P4Table *table) {
    return Tofino1TableExecutor(*table, *this).processTable();
}

// Provides implementations of Tofino externs.
namespace Tofino1Externs {
const ExternMethodImpls EXTERN_METHOD_IMPLS({});
}  // namespace Tofino1Externs

const IR::Expression *Tofino1ExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    auto method = Tofino1Externs::EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return TofinoBaseExpressionResolver::processExtern(externInfo);
}

}  // namespace P4::P4Tools::Flay::Tofino
