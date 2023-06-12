#include "backends/p4tools/modules/flay/targets/v1model/expression_resolver.h"

#include <functional>
#include <optional>

#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/targets/v1model/constants.h"
#include "backends/p4tools/modules/flay/targets/v1model/table_executor.h"
#include "ir/irutils.h"

namespace P4Tools::Flay::V1Model {

V1ModelExpressionResolver::V1ModelExpressionResolver(const ProgramInfo &programInfo,
                                                     ExecutionState &executionState)
    : ExpressionResolver(programInfo, executionState) {}

const IR::Expression *V1ModelExpressionResolver::processTable(const IR::P4Table *table) {
    auto copy = V1ModelExpressionResolver(*this);
    auto tableExecutor = V1ModelTableExecutor(*table, copy);
    return tableExecutor.processTable();
}

const IR::Expression *V1ModelExpressionResolver::processExtern(
    ExternMethodImpls::ExternInfo &externInfo) {
    // Provides implementations of BMv2 externs.
    static const ExternMethodImpls EXTERN_METHOD_IMPLS(
        {{"*method.mark_to_drop",
          {"standard_metadata"},
          [](ExternMethodImpls::ExternInfo &externInfo) {
              const auto *nineBitType = IR::getBitType(9);
              const auto *metadataLabel = externInfo.externArgs->at(0)->expression;
              if (!(metadataLabel->is<IR::Member>() || metadataLabel->is<IR::PathExpression>())) {
                  P4C_UNIMPLEMENTED("Drop input %1% of type %2% not supported", metadataLabel,
                                    metadataLabel->type);
              }
              // Use an assignment to set egress_spec to true.
              // This variable will be processed in the deparser.
              const auto *portVar = new IR::Member(nineBitType, metadataLabel, "egress_spec");
              externInfo.state.set(portVar,
                                   IR::getConstant(nineBitType, V1ModelConstants::DROP_PORT));
              return nullptr;
          }}});

    auto method = EXTERN_METHOD_IMPLS.find(externInfo.externObjectRef, externInfo.methodName,
                                           externInfo.externArgs);
    if (method.has_value()) {
        return method.value()(externInfo);
    }
    return ExpressionResolver::processExtern(externInfo);
}

}  // namespace P4Tools::Flay::V1Model
