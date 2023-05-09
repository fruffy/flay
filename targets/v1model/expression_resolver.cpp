#include "backends/p4tools/modules/flay/targets/v1model/expression_resolver.h"

#include <functional>
#include <optional>

#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/targets/v1model/table_executor.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

V1ModelExpressionResolver::V1ModelExpressionResolver(const ProgramInfo &programInfo,
                                                     ExecutionState &executionState)
    : ExpressionResolver(programInfo, executionState) {}

const IR::Expression *V1ModelExpressionResolver::processTable(const IR::P4Table *table) {
    auto copy = V1ModelExpressionResolver(*this);
    auto tableExecutor = V1ModelTableExecutor(copy);
    return tableExecutor.processTable(table);
}

const IR::Expression *V1ModelExpressionResolver::processExtern(
    const IR::PathExpression &externObjectRef, const IR::ID &methodName,
    const IR::Vector<IR::Argument> *args) {
    // Provides implementations of BMv2 externs.
    static const ExternMethodImpls EXTERN_METHOD_IMPLS(
        {{"*method.mark_to_drop",
          {"standard_metadata"},
          [](const IR::PathExpression & /*externObjectRef*/, const IR::ID & /*methodName*/,
             const IR::Vector<IR::Argument> *args, ExecutionState &state) {
              const auto *nineBitType = IR::getBitType(9);
              const auto *metadataLabel = args->at(0)->expression;
              if (!(metadataLabel->is<IR::Member>() || metadataLabel->is<IR::PathExpression>())) {
                  P4C_UNIMPLEMENTED("Drop input %1% of type %2% not supported", metadataLabel,
                                    metadataLabel->type);
              }
              // Use an assignment to set egress_spec to true.
              // This variable will be processed in the deparser.
              const auto *portVar = new IR::Member(nineBitType, metadataLabel, "egress_spec");
              state.set(portVar, IR::getConstant(nineBitType, 511));
              return nullptr;
          }}});

    auto method = EXTERN_METHOD_IMPLS.find(externObjectRef, methodName, args);
    if (method.has_value()) {
        return method.value()(externObjectRef, methodName, args, getExecutionState());
    }
    return ExpressionResolver::processExtern(externObjectRef, methodName, args);
}

}  // namespace P4Tools::Flay
