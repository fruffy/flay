
#include "backends/p4tools/modules/flay/core/expression_resolver.h"

#include <stddef.h>

#include <optional>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/gen_eq.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/externs.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
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
    result = getExecutionState().get(ToolsVariables::convertReference(path));
    return false;
}

const IR::Expression *ExpressionResolver::checkStructLike(const IR::Member *member) {
    std::vector<const IR::ID *> ids;
    const IR::Expression *expr = member;
    while (const auto *member = expr->to<IR::Member>()) {
        ids.emplace_back(&member->member);
        expr = member->expr;
    }
    // If the member is a method call, resolve it. The result MUST be a struct expression.
    if (const auto *methodCallExpr = expr->to<IR::MethodCallExpression>()) {
        expr = computeResult(methodCallExpr)->checkedTo<IR::StructExpression>();
    }
    // If the expression is a struct expression, try to resolve the member contained in it
    // iteratively.
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
    // Some members may access struct expression, not actual state. Resolve these first.
    const auto *structExpr = checkStructLike(member);
    if (structExpr != nullptr) {
        result = structExpr;
        return false;
    }
    result = getExecutionState().get(member);
    return false;
}

bool ExpressionResolver::preorder(const IR::ArrayIndex *arrIndex) {
    const auto *right = arrIndex->right;
    bool hasChanged = false;
    if (!SymbolicEnv::isSymbolicValue(right)) {
        right = computeResult(right);
        hasChanged = true;
    }
    if (hasChanged) {
        auto *newIndex = arrIndex->clone();
        newIndex->right = right;
        arrIndex = newIndex;
    }
    result = getExecutionState().get(arrIndex);
    return false;
}

bool ExpressionResolver::preorder(const IR::Operation_Unary *op) {
    const auto *expr = op->expr;
    bool hasChanged = false;
    if (!SymbolicEnv::isSymbolicValue(expr)) {
        expr = computeResult(expr);
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
        left = computeResult(left);
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(right)) {
        right = computeResult(right);
        hasChanged = true;
    }

    // Equals operations are a little special since they may involve complex objects such as lists.
    if (op->is<IR::Equ>() || op->is<IR::Neq>()) {
        // TODO: This is a workaround to support comparison between list and struct expressions.
        // Ideally, we should have proper support for this expression.
        if (auto leftStructExpr = left->to<IR::StructExpression>()) {
            IR::Vector<IR::Expression> components;
            for (auto structExpr : leftStructExpr->components) {
                components.push_back(structExpr->expression);
            }
            left = new IR::ListExpression(leftStructExpr->type, components);
        }
        if (auto rightStructExpr = right->to<IR::StructExpression>()) {
            IR::Vector<IR::Expression> components;
            for (auto structExpr : rightStructExpr->components) {
                components.push_back(structExpr->expression);
            }
            right = new IR::ListExpression(rightStructExpr->type, components);
        }
        result = GenEq::equate(left, right);
        if (op->is<IR::Neq>()) {
            result = new IR::LNot(result);
        }
        return false;
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
        e0 = computeResult(e0);
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(e1)) {
        e1 = computeResult(e1);
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(e2)) {
        e2 = computeResult(e2);
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

bool ExpressionResolver::preorder(const IR::Mux *mux) {
    const auto *cond = mux->e0;
    const auto *ifExpr = mux->e1;
    const auto *elseExpr = mux->e2;
    bool hasChanged = false;
    if (!SymbolicEnv::isSymbolicValue(cond)) {
        cond = computeResult(cond);
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(elseExpr)) {
        elseExpr = computeResult(elseExpr);
        hasChanged = true;
    }

    if (!SymbolicEnv::isSymbolicValue(ifExpr)) {
        // Execute the case where the condition is true.
        // TODO: This whole sequence is quite bug-prone. We should clean this up.
        auto &oldState = getExecutionState();
        auto &trueState = oldState.clone();
        trueState.pushExecutionCondition(cond);
        executionState = trueState;
        ifExpr = computeResult(ifExpr);
        oldState.merge(trueState);
        executionState = oldState;
        hasChanged = true;
    }

    if (hasChanged) {
        auto *newOp = mux->clone();
        newOp->e0 = cond;
        newOp->e1 = ifExpr;
        newOp->e2 = elseExpr;
        result = newOp;
        return false;
    }
    result = mux;
    return false;
}

bool ExpressionResolver::preorder(const IR::ListExpression *listExpr) {
    IR::Vector<IR::Expression> components;
    bool hasChanged = false;
    for (const auto *expr : listExpr->components) {
        bool fieldHasChanged = false;
        if (!SymbolicEnv::isSymbolicValue(expr)) {
            expr = computeResult(expr);
            fieldHasChanged = true;
        }
        if (fieldHasChanged) {
            components.push_back(expr);
            hasChanged = true;
        }
    }
    if (hasChanged) {
        auto *newListExpr = listExpr->clone();
        newListExpr->components = components;
        result = newListExpr;
        return false;
    }
    result = listExpr;
    return false;
}

bool ExpressionResolver::preorder(const IR::StructExpression *structExpr) {
    IR::IndexedVector<IR::NamedExpression> components;
    bool hasChanged = false;
    for (const auto *field : structExpr->components) {
        const auto *expr = field->expression;
        bool fieldHasChanged = false;
        if (!SymbolicEnv::isSymbolicValue(expr)) {
            expr = computeResult(expr);
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
    auto &state = getExecutionState();

    // Handle method calls. These are either table invocations or extern calls.
    if (call->method->type->is<IR::Type_Method>()) {
        // Assume that all cases of path expressions are extern calls.
        if (const auto *path = call->method->to<IR::PathExpression>()) {
            static auto METHOD_DUMMY =
                IR::PathExpression(new IR::Type_Extern("*method"), new IR::Path("*method"));
            result = processExtern(
                {*call, METHOD_DUMMY, path->path->name, call->arguments, executionState});
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
                result =
                    processExtern({*call, *path, method->member, call->arguments, executionState});
                return false;
            }

            // Handle table calls.
            if (const auto *table = state.findTable(method)) {
                result = processTable(table);
                return false;
            }

            // Handle calls to header methods.
            if (method->expr->type->is<IR::Type_Header>() ||
                method->expr->type->is<IR::Type_HeaderUnion>()) {
                if (method->member == "setValid") {
                    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(method->expr);
                    state.set(headerRefValidity, IR::getBoolLiteral(true));
                    return false;
                }
                if (method->member == "setInvalid") {
                    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(method->expr);
                    state.set(headerRefValidity, IR::getBoolLiteral(false));
                    return false;
                }
                if (method->member == "isValid") {
                    const auto &headerRefValidity = ToolsVariables::getHeaderValidity(method->expr);
                    result = state.get(headerRefValidity);
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
        const auto *actionType = state.getP4Action(call);
        TableExecutor::callAction(programInfo, executionState, actionType, *call->arguments);
        return false;
    }
    P4C_UNIMPLEMENTED("Unknown method call expression: %1%", call);
}

const IR::Expression *createSymbolicExpression(const ExecutionState &state,
                                               const IR::Type *inputType, cstring label,
                                               size_t id) {
    const auto *resolvedType = state.resolveType(inputType);
    if (const auto *structType = resolvedType->to<IR::Type_StructLike>()) {
        IR::IndexedVector<IR::NamedExpression> fields;
        for (size_t idx = 0; idx < structType->fields.size(); ++idx) {
            const auto *field = structType->fields.at(idx);
            fields.push_back(new IR::NamedExpression(
                field->name, createSymbolicExpression(state, field->type, label, idx + id)));
        }
        return new IR::StructExpression(inputType, fields);
    }
    if (const auto *stackType = resolvedType->to<IR::Type_Stack>()) {
        IR::Vector<IR::Expression> fields;
        for (size_t idx = 0; idx < stackType->getSize(); ++idx) {
            fields.push_back(
                createSymbolicExpression(state, stackType->elementType, label, idx + id));
        }
        return new IR::HeaderStackExpression(inputType, fields, inputType);
    }
    if (resolvedType->is<IR::Type_Base>()) {
        cstring labelId = label + " " + std::to_string(id);
        return ToolsVariables::getSymbolicVariable(resolvedType, labelId);
    }
    P4C_UNIMPLEMENTED("Requesting a symbolic expression for %1% of type %2%", inputType,
                      inputType->node_type_name());
}

/* =============================================================================================
 *  Extern implementations
 * ============================================================================================= */

const IR::Expression *ExpressionResolver::processExtern(
    const ExternMethodImpls::ExternInfo &externInfo) {
    // Provides implementations of P4 core externs.
    static const ExternMethodImpls EXTERN_METHOD_IMPLS({
        {"packet_in.extract",
         {"hdr"},
         [this](const ExternMethodImpls::ExternInfo &externInfo) {
             const auto *args = externInfo.externArgs;
             const auto &externObjectRef = externInfo.externObjectRef;
             const auto &methodName = externInfo.methodName;
             auto &state = getExecutionState();

             // This argument is the structure being written by the extract.
             const auto &extractRef = ToolsVariables::convertReference(args->at(0)->expression);
             const auto *headerType = extractRef->type->checkedTo<IR::Type_Header>();
             const auto &headerRefValidity = ToolsVariables::getHeaderValidity(extractRef);
             // First, set the validity.
             auto extractLabel =
                 externObjectRef.path->toString() + "_" + methodName + "_" + extractRef->toString();
             state.set(headerRefValidity,
                       ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), extractLabel));
             // Then, set the fields.
             const auto flatFields = state.getFlatFields(extractRef, headerType);
             for (const auto &field : flatFields) {
                 auto extractFieldLabel = externObjectRef.path->toString() + "_" + methodName +
                                          "_" + extractRef->toString() + "_" + field.toString();
                 state.set(field,
                           ToolsVariables::getSymbolicVariable(field->type, extractFieldLabel));
             }
             return nullptr;
         }},
        {"packet_in.extract",
         {"hdr", "sizeInBits"},
         [this](const ExternMethodImpls::ExternInfo &externInfo) {
             const auto *args = externInfo.externArgs;
             const auto &externObjectRef = externInfo.externObjectRef;
             const auto &methodName = externInfo.methodName;
             auto &state = getExecutionState();

             // This argument is the structure being written by the extract.
             const auto &extractRef = ToolsVariables::convertReference(args->at(0)->expression);
             const auto *headerType = extractRef->type->checkedTo<IR::Type_Header>();
             const auto &headerRefValidity = ToolsVariables::getHeaderValidity(extractRef);
             // First, set the validity.
             auto extractLabel =
                 externObjectRef.path->toString() + "_" + methodName + "_" + extractRef->toString();
             state.set(headerRefValidity,
                       ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), extractLabel));
             // Then, set the fields.
             const auto flatFields = state.getFlatFields(extractRef, headerType);
             for (const auto &field : flatFields) {
                 auto extractFieldLabel = externObjectRef.path->toString() + "_" + methodName +
                                          "_" + extractRef->toString() + "_" + field.toString();
                 // For now, we ignore the assigned size in our calculations and always use the
                 // maximum size.
                 // TODO: Figure out a way to exploit sizeInBits?
                 if (auto varbitType = field->type->to<IR::Extracted_Varbits>()) {
                     auto assignedType = varbitType->clone();
                     assignedType->assignedSize = assignedType->size;
                     auto typedField = field.clone();
                     typedField->type = assignedType;
                     state.set(*typedField, ToolsVariables::getSymbolicVariable(typedField->type,
                                                                                extractFieldLabel));
                 } else {
                     state.set(field,
                               ToolsVariables::getSymbolicVariable(field->type, extractFieldLabel));
                 }
             }
             return nullptr;
         }},
        {"packet_in.lookahead",
         {},
         [this](const ExternMethodImpls::ExternInfo &externInfo) {
             auto &state = getExecutionState();
             const auto *typeArgs = externInfo.originalCall.typeArguments;
             BUG_CHECK(typeArgs->size() == 1, "Lookahead should have exactly one type argument.");
             // TODO: We currently just create a dummy variable, but this is not correct.
             // Since we look ahead, subsequent extracts and advance calls are influenced by the
             // decision.
             const auto *lookaheadType = externInfo.originalCall.typeArguments->at(0);
             const auto &externObjectRef = externInfo.externObjectRef;
             const auto &methodName = externInfo.methodName;
             auto lookaheadLabel = externObjectRef.path->toString() + "_" + methodName + "_" +
                                   std::to_string(externInfo.originalCall.clone_id);
             return createSymbolicExpression(state, lookaheadType, lookaheadLabel, 0);
         }},
        {"packet_in.advance",
         {"sizeInBits"},
         [](const ExternMethodImpls::ExternInfo & /*externInfo*/) {
             // Advance is a no-op for now.
             return nullptr;
         }},
        {"packet_out.emit",
         {"hdr"},
         [](const ExternMethodImpls::ExternInfo & /*externInfo*/) {
             // Emit is a no-op for now.
             return nullptr;
         }},
        /* ======================================================================================
         *  verify
         *  The verify statement provides a simple form of error handling.
         *  If the first argument is true, then executing the statement has no side-effect.
         *  However, if the first argument is false, it causes an immediate transition to
         *  reject, which causes immediate parsing termination; at the same time, the
         *  parserError associated with the parser is set to the value of the second
         *  argument.
         * ======================================================================================
         */
        {"*method.verify",
         {"bool", "error"},
         [](const ExternMethodImpls::ExternInfo & /*externInfo*/) {
             // TODO: Implement the error case.
             return nullptr;
         }},
    });
    auto method = EXTERN_METHOD_IMPLS.find(externInfo.externObjectRef, externInfo.methodName,
                                           externInfo.externArgs);
    if (method.has_value()) {
        return std::bind(method.value(), std::ref(externInfo))();
    }
    P4C_UNIMPLEMENTED("Unknown or unimplemented extern method: %1%.%2%",
                      externInfo.externObjectRef.toString(), externInfo.methodName);
}

const IR::Expression *ExpressionResolver::computeResult(const IR::Node *node) {
    node->apply_visitor_preorder(*this);
    return getResult();
}

}  // namespace P4Tools::Flay
