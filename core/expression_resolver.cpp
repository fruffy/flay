
#include "backends/p4tools/modules/flay/core/expression_resolver.h"

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"
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

bool ExpressionResolver::preorder(const IR::Literal *lit) {
    result = lit;
    return false;
}

bool ExpressionResolver::preorder(const IR::PathExpression *path) {
    result = getExecutionState().get(StateUtils::convertReference(path));
    return false;
}

bool ExpressionResolver::preorder(const IR::Member *member) {
    result = getExecutionState().get(member);
    return false;
}

bool ExpressionResolver::preorder(const IR::Operation_Unary *op) {
    const auto *expr = op->expr;
    bool hasChanged = false;
    if (!SymbolicEnv::isSymbolicValue(expr)) {
        expr->apply_visitor_preorder(*this);
        expr = getResult();
        hasChanged = true;
    }

    if (hasChanged) {
        auto *newOp = op->clone();
        newOp->expr = expr;
        result = newOp;
        return false;
    }
    result = op;
    return false;
}

bool ExpressionResolver::preorder(const IR::Operation_Binary *op) {
    const auto *left = op->left;
    const auto *right = op->right;
    bool hasChanged = false;
    if (!SymbolicEnv::isSymbolicValue(left)) {
        left->apply_visitor_preorder(*this);
        left = getResult();
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(right)) {
        right->apply_visitor_preorder(*this);
        right = getResult();
        hasChanged = true;
    }
    if (hasChanged) {
        auto *newOp = op->clone();
        newOp->left = left;
        newOp->right = right;
        result = newOp;
        return false;
    }
    result = op;
    return false;
}

bool ExpressionResolver::preorder(const IR::Operation_Ternary *op) {
    const auto *e0 = op->e0;
    const auto *e1 = op->e1;
    const auto *e2 = op->e2;
    bool hasChanged = false;
    if (!SymbolicEnv::isSymbolicValue(e0)) {
        e0->apply_visitor_preorder(*this);
        e0 = getResult();
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(e1)) {
        e1->apply_visitor_preorder(*this);
        e1 = getResult();
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(e2)) {
        e2->apply_visitor_preorder(*this);
        e2 = getResult();
        hasChanged = true;
    }
    if (hasChanged) {
        auto *newOp = op->clone();
        newOp->e0 = e0;
        newOp->e1 = e1;
        newOp->e2 = e2;
        result = newOp;
        return false;
    }
    result = op;
    return false;
}

bool ExpressionResolver::preorder(const IR::StructExpression *structExpr) {
    IR::IndexedVector<IR::NamedExpression> components;
    bool hasChanged = false;
    for (const auto *field : structExpr->components) {
        const auto *expr = field->expression;
        bool fieldHasChanged = false;
        if (!SymbolicEnv::isSymbolicValue(expr)) {
            expr->apply_visitor_preorder(*this);
            expr = getResult();
            fieldHasChanged = true;
        }
        if (fieldHasChanged) {
            auto *newField = field->clone();
            newField->expression = expr;
            components.push_back(newField);
            hasChanged = true;
        }
    }
    if (hasChanged) {
        auto *newStructExpr = structExpr->clone();
        newStructExpr->components = components;
        result = newStructExpr;
        return false;
    }
    result = structExpr;
    return false;
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

            // Handle table calls.
            if (const auto *table = StateUtils::findTable(executionState, method)) {
                TableExecutor executor(programInfo, executionState);
                table->apply(executor);
                result = executor.getResult();
                return false;
            }

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

                P4C_UNIMPLEMENTED("Unknown method call on header instance: %1%", call);
            }

            P4C_UNIMPLEMENTED("Unknown method member expression: %1% of type %2%", method->expr,
                              method->expr->type);
        }

        P4C_UNIMPLEMENTED("Unknown method call: %1% of type %2%", call->method,
                          call->method->node_type_name());
    }
    P4C_UNIMPLEMENTED("Unknown method call expression: %1%", call);
}

}  // namespace P4Tools::Flay
