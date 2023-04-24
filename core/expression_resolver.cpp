
#include "backends/p4tools/modules/flay/core/expression_resolver.h"

#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

ExpressionResolver::ExpressionResolver(const ProgramInfo &programInfo,
                                       ExecutionState &executionState)
    : programInfo(programInfo), executionState(executionState) {}

const ProgramInfo &ExpressionResolver::getProgramInfo() const { return programInfo.get(); }

ExecutionState &ExpressionResolver::getExecutionState() const { return executionState.get(); }

const IR::Expression *ExpressionResolver::getResult() {
    CHECK_NULL(result);
    return result;
}

/* =============================================================================================
 *  Visitor functions
 * ============================================================================================= */

bool ExpressionResolver::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Node %1% of type %2% not implemented in the expression resolver.", node,
                      node->node_type_name());
}

bool ExpressionResolver::preorder(const IR::MethodCallExpression *call) {
    auto &executionState = getExecutionState();

    // Handle method calls. These are either table invocations or extern calls.
    if (call->method->type->is<IR::Type_Method>()) {
        if (const auto *method = call->method->to<IR::Member>()) {
            // Case where call->method is a Member expression. For table invocations, the
            // qualifier of the member determines the table being invoked. For extern calls,
            // the qualifier determines the extern object containing the method being invoked.
            BUG_CHECK(method->expr, "Method call has unexpected format: %1%", call);

            // Handle calls to header methods.
            if (method->expr->type->is<IR::Type_Header>() ||
                method->expr->type->is<IR::Type_HeaderUnion>()) {
                if (method->member == "setValid") {
                    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(method->expr);
                    executionState.set(headerRefValidity, IR::getBoolLiteral(true));
                    return false;
                }
                if (method->member == "setInvalid") {
                    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(method->expr);
                    executionState.set(headerRefValidity, IR::getBoolLiteral(false));
                    return false;
                }

                BUG("Unknown method call on header instance: %1%", call);
            }

            BUG("Unknown method member expression: %1% of type %2%", method->expr,
                method->expr->type);
        }

        BUG("Unknown method call: %1% of type %2%", call->method, call->method->node_type_name());
    }
    BUG("Unknown method call expression: %1%", call);
}

}  // namespace P4Tools::Flay
