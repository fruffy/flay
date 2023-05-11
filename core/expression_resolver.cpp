
#include "backends/p4tools/modules/flay/core/expression_resolver.h"

#include <optional>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/externs.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"

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

const IR::Expression *checkStructLike(const IR::Member *member) {
    std::vector<const IR::ID *> ids;
    const IR::Expression *expr = member;
    while (const auto *member = expr->to<IR::Member>()) {
        ids.emplace_back(&member->member);
        expr = member->expr;
    }
    if (const auto *structExpr = expr->to<IR::StructExpression>()) {
        while (!ids.empty()) {
            const auto *ref = ids.back();
            ids.pop_back();
            const auto *expr = structExpr->getField(*ref)->expression;
            if (const auto *se = expr->to<IR::StructExpression>()) {
                structExpr = se;
            } else {
                return expr;
            }
        }
    }
    return nullptr;
}

bool ExpressionResolver::preorder(const IR::Member *member) {
    // const auto *structExpr = checkStructLike(member);
    // if (structExpr != nullptr) {
    //     result = structExpr;
    //     return false;
    // }
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
        // Assume that all cases of path expressions are extern calls.
        if (const auto *path = call->method->to<IR::PathExpression>()) {
            static auto METHOD_DUMMY =
                IR::PathExpression(new IR::Type_Extern("*method"), new IR::Path("*method"));
            processExtern(METHOD_DUMMY, path->path->name, call->arguments);
            return false;
        }

        if (const auto *method = call->method->to<IR::Member>()) {
            // Case where call->method is a Member expression. For table invocations, the
            // qualifier of the member determines the table being invoked. For extern calls,
            // the qualifier determines the extern object containing the method being invoked.
            BUG_CHECK(method->expr, "Method call has unexpected format: %1%", call);

            // Handle extern calls. They may also be of Type_SpecializedCanonical.
            if (method->expr->type->is<IR::Type_Extern>() ||
                method->expr->type->is<IR::Type_SpecializedCanonical>()) {
                const auto *path = method->expr->checkedTo<IR::PathExpression>();
                processExtern(*path, method->member, call->arguments);
                return false;
            }

            // Handle table calls.
            if (const auto *table = StateUtils::findTable(executionState, method)) {
                result = processTable(table);
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
                if (method->member == "isValid") {
                    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(method->expr);
                    result = executionState.get(headerRefValidity);
                    return false;
                }
                P4C_UNIMPLEMENTED("Unknown method call on header instance: %1%", call);
            }

            P4C_UNIMPLEMENTED("Unknown method member expression: %1% of type %2%", method->expr,
                              method->expr->type);
        }

        P4C_UNIMPLEMENTED("Unknown method call: %1% of type %2%", call->method,
                          call->method->node_type_name());
    } else if (call->method->type->is<IR::Type_Action>()) {
        // Handle action calls. Actions are called by tables and are not inlined, unlike
        // functions.
        const auto *actionType = StateUtils::getP4Action(executionState, call);
        TableExecutor::callAction(programInfo, executionState, actionType, *call->arguments);
        return false;
    }
    P4C_UNIMPLEMENTED("Unknown method call expression: %1%", call);
}

/* =============================================================================================
 *  Extern implementations
 * ============================================================================================= */

const IR::Expression *ExpressionResolver::processExtern(const IR::PathExpression &externObjectRef,
                                                        const IR::ID &methodName,
                                                        const IR::Vector<IR::Argument> *args) {
    // Provides implementations of P4 core externs.
    static const ExternMethodImpls EXTERN_METHOD_IMPLS({
        {"packet_in.extract",
         {"hdr"},
         [](const IR::PathExpression &externObjectRef, const IR::ID &methodName,
            const IR::Vector<IR::Argument> *args, ExecutionState &state) {
             // This argument is the structure being written by the extract.
             const auto &extractRef = StateUtils::convertReference(args->at(0)->expression);
             const auto *headerType = extractRef->type->checkedTo<IR::Type_Header>();
             const auto &headerRefValidity = ToolsVariables::getHeaderValidity(extractRef);
             // First, set the validity.
             auto extractLabel =
                 externObjectRef.path->toString() + "_" + methodName + "_" + extractRef->toString();
             state.set(headerRefValidity, ToolsVariables::getSymbolicVariable(
                                              IR::Type_Boolean::get(), 0, extractLabel));
             // Then, set the fields.
             const auto flatFields = StateUtils::getFlatFields(state, extractRef, headerType);
             for (const auto &field : flatFields) {
                 auto extractFieldLabel = externObjectRef.path->toString() + "_" + methodName +
                                          "_" + extractRef->toString() + "_" + field.toString();
                 state.set(field,
                           ToolsVariables::getSymbolicVariable(field->type, 0, extractFieldLabel));
             }
             return nullptr;
         }},
        {"packet_out.emit",
         {"hdr"},
         [](const IR::PathExpression & /*externObjectRef*/, const IR::ID & /*methodName*/,
            const IR::Vector<IR::Argument> * /*args*/,
            ExecutionState & /*state*/) { return nullptr; }},
    });

    auto method = EXTERN_METHOD_IMPLS.find(externObjectRef, methodName, args);
    if (method.has_value()) {
        return method.value()(externObjectRef, methodName, args, getExecutionState());
    }
    P4C_UNIMPLEMENTED("Unknown or unimplemented extern method: %1%.%2%", externObjectRef.toString(),
                      methodName);
}

}  // namespace P4Tools::Flay
