#include "backends/p4tools/modules/flay/targets/nikss/psa/expression_resolver.h"

#include <functional>
#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/interpreter/externs.h"
#include "backends/p4tools/modules/flay/targets/nikss/psa/table_executor.h"
#include "ir/irutils.h"

namespace P4::P4Tools::Flay::Nikss {

PsaExpressionResolver::PsaExpressionResolver(const ProgramInfo &programInfo,
                                             ControlPlaneConstraints &constraints,
                                             ExecutionState &executionState)
    : NikssBaseExpressionResolver(programInfo, constraints, executionState) {}

const IR::Expression *PsaExpressionResolver::processTable(const IR::P4Table *table) {
    return PsaTableExecutor(*table, *this).processTable();
}

// Provides implementations of Nikss externs.
namespace PsaExterns {

using namespace P4::literals;

const ExternMethodImpls EXTERN_METHOD_IMPLS({
    /* ======================================================================================
     *  UserExtern.apply
     * Unknown behavior. The only thing that is known that the second parameter can be written to.
     * ======================================================================================
     */
    {"UserExtern.apply"_cs,
     {"extern_in"_cs, "extern_out"_cs},
     [](const ExternMethodImpls::ExternInfo &externInfo) {
         auto &state = externInfo.state;

         // TODO: Implement and actually keep track of the writes.
         const auto &resultVar =
             externInfo.externArgs->at(1)->expression->checkedTo<IR::InOutReference>()->ref;

         auto registerLabel = externInfo.externObjectRef.path->toString() + "_" +
                              externInfo.methodName + "_" +
                              std::to_string(externInfo.originalCall.clone_id);
         const auto *hashCalc = ToolsVariables::getSymbolicVariable(resultVar->type, registerLabel);
         state.set(resultVar, hashCalc);
         return nullptr;
     }},
});
}  // namespace PsaExterns

const IR::Expression *PsaExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    auto method = PsaExterns::EXTERN_METHOD_IMPLS.find(
        externInfo.externObjectRef, externInfo.methodName, externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return NikssBaseExpressionResolver::processExtern(externInfo);
}

}  // namespace P4::P4Tools::Flay::Nikss
