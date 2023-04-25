
#include "backends/p4tools/modules/flay/core/table_executor.h"

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

TableExecutor::TableExecutor(const ProgramInfo &programInfo, ExecutionState &executionState)
    : programInfo(programInfo), executionState(executionState) {}

const ProgramInfo &TableExecutor::getProgramInfo() const { return programInfo.get(); }

ExecutionState &TableExecutor::getExecutionState() const { return executionState.get(); }

const IR::Expression *TableExecutor::getResult() {
    CHECK_NULL(result);
    return result;
}

const IR::Key *TableExecutor::resolveKey(const IR::Key *key) const {
    IR::Vector<IR::KeyElement> keyElements;
    bool hasChanged = false;
    ExpressionResolver resolver(programInfo, executionState);
    for (const auto *keyField : key->keyElements) {
        const auto *expr = keyField->expression;
        bool keyFieldHasChanged = false;
        if (!SymbolicEnv::isSymbolicValue(expr)) {
            expr->apply(resolver);
            expr = resolver.getResult();
            keyFieldHasChanged = true;
        }
        if (keyFieldHasChanged) {
            auto *newKeyField = keyField->clone();
            newKeyField->expression = expr;
            keyElements.push_back(newKeyField);
            hasChanged = true;
        }
    }
    if (hasChanged) {
        auto *newKey = key->clone();
        newKey->keyElements = keyElements;
        return newKey;
    }
    return key;
}

std::vector<const IR::ActionListElement *> TableExecutor::buildTableActionList(
    const IR::P4Table *table) {
    std::vector<const IR::ActionListElement *> tableActionList;
    const auto *actionList = table->getActionList();
    if (actionList == nullptr) {
        return tableActionList;
    }
    for (size_t idx = 0; idx < actionList->size(); idx++) {
        const auto *action = actionList->actionList.at(idx);
        if (action->getAnnotation("defaultonly") != nullptr) {
            continue;
        }
        // Check some properties of the list.
        CHECK_NULL(action->expression);
        action->expression->checkedTo<IR::MethodCallExpression>();
        tableActionList.emplace_back(action);
    }
    return tableActionList;
}

// class P4Constants {
//  public:
//     // Parser error codes, copied from core.p4.
//     /// No error.
//     static constexpr int NO_ERROR = 0x0000;
//     /// Not enough bits in packet for 'extract'.
//     static constexpr int PARSER_ERROR_PACKET_TOO_SHORT = 0x0001;
//     /// 'select' expression has no matches
//     static constexpr int PARSER_ERROR_NO_MATCH = 0x0002;
//     /// Reference to invalid element of a header stack.
//     static constexpr int PARSER_ERROR_STACK_OUT_OF_BOUNDS = 0x0003;
//     /// Extracting too many bits into a varbit field.
//     static constexpr int PARSER_ERROR_HEADER_TOO_SHORT = 0x0004;
//     /// Parser execution time limit exceeded.
//     static constexpr int PARSER_ERROR_TIMEOUT = 0x005;
//     /// Parser operation was called with a value
//     /// not supported by the implementation.
//     static constexpr int PARSER_ERROR_INVALID_ARGUMENT = 0x0020;
//     /// Match bits exactly.
//     static constexpr const char *MATCH_KIND_EXACT = "exact";
//     /// Ternary match, using a mask.
//     static constexpr const char *MATCH_KIND_TERNARY = "ternary";
//     /// Longest-prefix match.
//     static constexpr const char *MATCH_KIND_LPM = "lpm";
// };

// const IR::Expression *computeTargetMatchType(ExecutionState &state, const IR::P4Table *table,
//                                              const IR::Key *key) {
//     auto tableName = table->controlPlaneName();
//     const IR::Expression *hitCondition = IR::getBoolLiteral(true);
//     for (const auto *keyField : key->keyElements) {
//         const auto *keyExpr = keyField->expression;
//         const auto matchType = keyField->matchType->toString();
//         const auto *nameAnnot = keyField->getAnnotation("name");
//         bool isTainted = false;
//         // Some hidden tables do not have any key name annotations.
//         BUG_CHECK(nameAnnot != nullptr /* || properties.tableIsImmutable*/,
//                   "Non-constant table key without an annotation");
//         cstring fieldName;
//         if (nameAnnot != nullptr) {
//             fieldName = nameAnnot->getName();
//         }
//         // Create a new variable constant that corresponds to the key expression.
//         cstring keyName = tableName + "_key_" + fieldName;
//         const auto *ctrlPlaneKey = ToolsVariables::getSymbolicVariable(keyExpr->type, 0,
//         keyName);

//         if (matchType == P4Constants::MATCH_KIND_EXACT) {
//             hitCondition = new IR::LAnd(hitCondition, new IR::Equ(keyExpr, ctrlPlaneKey));
//             return hitCondition;
//         }
//         if (matchType == P4Constants::MATCH_KIND_TERNARY) {
//             cstring maskName = tableName + "_mask_" + fieldName;
//             const IR::Expression *ternaryMask = nullptr;
//             // We can recover from taint by inserting a ternary match that is 0.
//             if (isTainted) {
//                 ternaryMask = IR::getConstant(keyExpr->type, 0);
//                 keyExpr = ternaryMask;
//             } else {
//                 ternaryMask = ToolsVariables::getSymbolicVariable(keyExpr->type, 0, maskName);
//             }
//             return new IR::LAnd(hitCondition, new IR::Equ(new IR::BAnd(keyExpr, ternaryMask),
//                                                           new IR::BAnd(ctrlPlaneKey,
//                                                           ternaryMask)));
//         }
//         if (matchType == P4Constants::MATCH_KIND_LPM) {
//             const auto *keyType = keyExpr->type->checkedTo<IR::Type_Bits>();
//             auto keyWidth = keyType->width_bits();
//             cstring maskName = tableName + "_lpm_prefix_" + fieldName;
//             const IR::Expression *maskVar =
//                 ToolsVariables::getSymbolicVariable(keyExpr->type, 0, maskName);
//             // The maxReturn is the maximum vale for the given bit width. This value is shifted
//             by
//             // the mask variable to create a mask (and with that, a prefix).
//             auto maxReturn = IR::getMaxBvVal(keyWidth);
//             auto *prefix = new IR::Sub(IR::getConstant(keyType, keyWidth), maskVar);
//             const IR::Expression *lpmMask = nullptr;
//             // We can recover from taint by inserting a ternary match that is 0.
//             if (isTainted) {
//                 lpmMask = IR::getConstant(keyExpr->type, 0);
//                 maskVar = lpmMask;
//                 keyExpr = lpmMask;
//             } else {
//                 lpmMask = new IR::Shl(IR::getConstant(keyType, maxReturn), prefix);
//             }
//             return new IR::LAnd(
//                 hitCondition,
//                 new IR::LAnd(
//                     // This is the actual LPM match under the shifted mask (the prefix).
//                     new IR::Leq(maskVar, IR::getConstant(keyType, keyWidth)),
//                     // The mask variable shift should not be larger than the key width.
//                     new IR::Equ(new IR::BAnd(keyExpr, lpmMask),
//                                 new IR::BAnd(ctrlPlaneKey, lpmMask))));
//         }

//         P4C_UNIMPLEMENTED("Match type %s not implemented for table keys.", matchType);
//     }
// }

// const IR::Expression *computeHit(ExecutionState &state) {
//     const IR::Expression *hitCondition = IR::getBoolLiteral(!properties.resolvedKeys.empty());
//     hitCondition = computeTargetMatchType(state, keyProperties, matches, hitCondition);
//     return hitCondition;
// }

/* =============================================================================================
 *  Visitor functions
 * =============================================================================================
 */

bool TableExecutor::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Node %1% of type %2% not implemented in the expression resolver.", node,
                      node->node_type_name());
}

bool TableExecutor::preorder(const IR::P4Table *table) {
    // Dummy the result until we have implemented the return struct.
    result = IR::getBoolLiteral(true);

    // First, resolve the key.
    const auto *key = table->getKey();
    if (key == nullptr) {
        return false;
    }
    key = resolveKey(key);
    auto tableActionList = buildTableActionList(table);
    auto &state = getExecutionState();
    for (const auto *action : tableActionList) {
        auto &actionState = state.clone();
        auto &actionStepper = FlayTarget::getStepper(programInfo, actionState);
        // Grab the path from the method call.
        const auto *tableAction = action->expression->checkedTo<IR::MethodCallExpression>();
        // Try to find the action declaration corresponding to the path reference in the table.
        const auto *path = tableAction->method->to<IR::PathExpression>();
        const auto *declaration = state.findDecl(path);
        const auto *actionType = declaration->checkedTo<IR::P4Action>();

        // First, we compute the hit condition to trigger this particular action call.
        const auto *hitCondition = IR::getBoolLiteral(true);

        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        for (size_t argIdx = 0; argIdx < parameters->size(); ++argIdx) {
            const auto *parameter = parameters->getParameter(argIdx);
            const auto *paramType = state.resolveType(parameter->type);
            // Synthesize a variable constant here that corresponds to a control plane argument.
            // We get the unique name of the table coupled with the unique name of the action.
            // Getting the unique name is needed to avoid generating duplicate arguments.
            cstring paramName =
                table->controlPlaneName() + "_" + actionName + "_" + parameter->controlPlaneName();
            const auto &actionArg =
                ToolsVariables::getSymbolicVariable(parameter->type, 0, paramName);
            const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(parameter->name));
            actionState.set(paramRef, actionArg);
        }
        actionType->body->apply(actionStepper);
        state.merge(actionState.getSymbolicEnv(), hitCondition);
    }
    return false;
}

}  // namespace P4Tools::Flay
