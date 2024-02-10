
#include "backends/p4tools/modules/flay/core/table_executor.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/modules/flay/core/collapse_mux.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

TableExecutor::TableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver)
    : table(table), resolver(callingResolver) {}

const ProgramInfo &TableExecutor::getProgramInfo() const { return resolver.get().getProgramInfo(); }

ExecutionState &TableExecutor::getExecutionState() const {
    return resolver.get().getExecutionState();
}

const IR::P4Table &TableExecutor::getP4Table() const { return table; }

const IR::Key *TableExecutor::resolveKey(const IR::Key *key) const {
    IR::Vector<IR::KeyElement> keyElements;
    bool hasChanged = false;
    for (const auto *keyField : key->keyElements) {
        const auto *expr = keyField->expression;
        bool keyFieldHasChanged = false;
        if (!SymbolicEnv::isSymbolicValue(expr)) {
            expr = resolver.get().computeResult(expr);
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

const IR::Expression *TableExecutor::computeKey(const IR::Key *key) const {
    const IR::Expression *hitCondition = IR::getBoolLiteral(true);
    for (const auto *keyField : key->keyElements) {
        const auto *matchExpr = computeTargetMatchType(keyField);
        hitCondition = new IR::LAnd(hitCondition, matchExpr);
    }
    return hitCondition;
}

const IR::Expression *TableExecutor::computeTargetMatchType(const IR::KeyElement *keyField) const {
    auto tableName = getP4Table().controlPlaneName();
    const auto *keyExpr = keyField->expression;
    const auto matchType = keyField->matchType->toString();
    const auto *nameAnnot = keyField->getAnnotation("name");
    bool isTainted = false;
    // Some hidden tables do not have any key name annotations.
    BUG_CHECK(nameAnnot != nullptr /* || properties.tableIsImmutable*/,
              "Non-constant table key without an annotation");
    cstring fieldName;
    if (nameAnnot != nullptr) {
        fieldName = nameAnnot->getName();
    }
    // Create a new variable constant that corresponds to the key expression.
    const auto *ctrlPlaneKey = ControlPlaneState::getTableKey(tableName, fieldName, keyExpr->type);

    if (matchType == P4Constants::MATCH_KIND_EXACT) {
        return new IR::Equ(keyExpr, ctrlPlaneKey);
    }
    if (matchType == P4Constants::MATCH_KIND_TERNARY) {
        const IR::Expression *ternaryMask = nullptr;
        // We can recover from taint by inserting a ternary match that is 0.
        if (isTainted) {
            ternaryMask = IR::getConstant(keyExpr->type, 0);
            keyExpr = ternaryMask;
        } else {
            ternaryMask =
                ControlPlaneState::getTableTernaryMask(tableName, fieldName, keyExpr->type);
        }
        return new IR::Equ(new IR::BAnd(keyExpr, ternaryMask),
                           new IR::BAnd(ctrlPlaneKey, ternaryMask));
    }
    if (matchType == P4Constants::MATCH_KIND_LPM) {
        const auto *keyType = keyExpr->type->checkedTo<IR::Type_Bits>();
        auto keyWidth = keyType->width_bits();
        const IR::Expression *maskVar =
            ControlPlaneState::getTableMatchLpmPrefix(tableName, fieldName, keyExpr->type);
        // The maxReturn is the maximum vale for the given bit width. This value is shifted by
        // the mask variable to create a mask (and with that, a prefix).
        auto maxReturn = IR::getMaxBvVal(keyWidth);
        auto *prefix = new IR::Sub(IR::getConstant(keyType, keyWidth), maskVar);
        const IR::Expression *lpmMask = nullptr;
        // We can recover from taint by inserting a ternary match that is 0.
        if (isTainted) {
            lpmMask = IR::getConstant(keyExpr->type, 0);
            maskVar = lpmMask;
            keyExpr = lpmMask;
        } else {
            lpmMask = new IR::Shl(IR::getConstant(keyType, maxReturn), prefix);
        }
        return new IR::LAnd(
            // This is the actual LPM match under the shifted mask (the prefix).
            new IR::Leq(maskVar, IR::getConstant(keyType, keyWidth)),
            // The mask variable shift should not be larger than the key width.
            new IR::Equ(new IR::BAnd(keyExpr, lpmMask), new IR::BAnd(ctrlPlaneKey, lpmMask)));
    }
    P4C_UNIMPLEMENTED("Match type %s not implemented for table keys.", matchType);
}

void TableExecutor::callAction(const ProgramInfo &programInfo, ExecutionState &state,
                               const IR::P4Action *actionType,
                               const IR::Vector<IR::Argument> &arguments) {
    const auto *parameters = actionType->parameters;
    BUG_CHECK(
        arguments.size() == parameters->parameters.size(),
        "Method call does not have the same number of arguments as the action has parameters.");
    for (size_t argIdx = 0; argIdx < parameters->size(); ++argIdx) {
        const auto *parameter = parameters->getParameter(argIdx);
        const auto *paramType = state.resolveType(parameter->type);
        // Synthesize a variable constant here that corresponds to a control plane argument.
        // We get the unique name of the table coupled with the unique name of the action.
        // Getting the unique name is needed to avoid generating duplicate arguments.
        const auto *actionArg = arguments.at(argIdx)->expression;
        const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(parameter->name));
        state.set(paramRef, actionArg);
    }
    auto &actionStepper = FlayTarget::getStepper(programInfo, state);
    actionType->body->apply(actionStepper);
}

void TableExecutor::processDefaultAction() const {
    auto &state = getExecutionState();
    const auto *defaultAction = getP4Table().getDefaultAction();
    const auto *tableAction = defaultAction->checkedTo<IR::MethodCallExpression>();
    const auto *actionType = state.getP4Action(tableAction);

    // Synthesize arguments for the call based on the action parameters.
    const auto *arguments = tableAction->arguments;
    callAction(getProgramInfo(), state, actionType, *arguments);
}

TableExecutor::ReturnProperties TableExecutor::processTableActionOptions(
    const IR::SymbolicVariable *tableActionID, const IR::Key *key) const {
    auto table = getP4Table();
    auto tableActionList = TableUtils::buildTableActionList(table);
    auto &state = getExecutionState();

    // First, we compute the hit condition to trigger this particular action call.
    const auto *hitCondition = computeKey(key);
    const auto *actionPath = TableUtils::getDefaultActionName(table);
    /// Pay attention to how we use "toString" for the path name here.
    /// We need to match these choices correctly. TODO: Make this very explicit.
    ReturnProperties retProperties{
        new IR::LAnd(ControlPlaneState::getTableActive(table.controlPlaneName()), hitCondition),
        new IR::StringLiteral(IR::Type_String::get(), actionPath->path->toString())};
    for (const auto *action : tableActionList) {
        const auto *actionType =
            state.getP4Action(action->expression->checkedTo<IR::MethodCallExpression>());
        auto *actionChoice = new IR::Equ(
            tableActionID,
            new IR::StringLiteral(IR::Type_String::get(), actionType->controlPlaneName()));
        const auto *actionHitCondition = new IR::LAnd(hitCondition, actionChoice);
        // We use action->controlPlaneName() here, NOT actionType. TODO: Clean this up?
        retProperties.actionRun = CollapseMux::produceOptimizedMux(
            actionHitCondition,
            new IR::StringLiteral(IR::Type_String::get(), action->controlPlaneName()),
            retProperties.actionRun);
        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        // Synthesize arguments for the call based on the action parameters.
        const auto &parameters = actionType->parameters;
        auto &actionState = state.clone();
        actionState.pushExecutionCondition(actionHitCondition);
        IR::Vector<IR::Argument> arguments;
        for (const auto *parameter : parameters->parameters) {
            // Synthesize a variable constant here that corresponds to a control plane argument.
            // We get the unique name of the table coupled with the unique name of the action.
            // Getting the unique name is needed to avoid generating duplicate arguments.
            const auto *actionArg = ControlPlaneState::getTableActionArgument(
                getP4Table().controlPlaneName(), actionName, parameter->controlPlaneName(),
                parameter->type);
            arguments.push_back(new IR::Argument(actionArg));
        }
        state.addReachabilityMapping(action, actionHitCondition);
        callAction(getProgramInfo(), actionState, actionType, arguments);
        // Finally, merge in the state of the action call.
        state.merge(actionState);
    }
    return retProperties;
}

TableExecutor::ReturnProperties TableExecutor::processConstantTableEntries(
    const IR::Key *key) const {
    auto table = getP4Table();
    auto tableActionList = TableUtils::buildTableActionList(table);
    auto &state = getExecutionState();

    // First, we compute the hit condition to trigger this particular action call.
    const auto *actionPath = TableUtils::getDefaultActionName(getP4Table());
    /// Pay attention to how we use "toString" for the path name here.
    /// We need to match these choices correctly. TODO: Make this very explicit.
    ReturnProperties retProperties{
        IR::getBoolLiteral(false),
        new IR::StringLiteral(IR::Type_String::get(), actionPath->path->toString())};

    const auto *entries = table.getEntries();

    // Sometimes, there are no entries. Just return.
    if (entries == nullptr) {
        return retProperties;
    }

    auto entryVector = entries->entries;

    // Sort entries if one of the keys contains an LPM match.
    for (size_t idx = 0; idx < key->keyElements.size(); ++idx) {
        const auto *keyElement = key->keyElements.at(idx);
        if (keyElement->matchType->path->toString() == P4Constants::MATCH_KIND_LPM) {
            std::sort(entryVector.begin(), entryVector.end(), [idx](auto &&PH1, auto &&PH2) {
                return TableUtils::compareLPMEntries(std::forward<decltype(PH1)>(PH1),
                                                     std::forward<decltype(PH2)>(PH2), idx);
            });
            break;
        }
    }
    for (const auto *entry : entryVector) {
        // First, compute the condition to match on the table entry.
        const auto *hitCondition = TableUtils::computeEntryMatch(table, *entry, *key);

        // Once we have computed the match, execution the action with its arguments.
        const auto *action = entry->getAction();
        const auto *actionCall = action->checkedTo<IR::MethodCallExpression>();
        const auto *actionType = state.getP4Action(actionCall);
        auto &actionState = state.clone();
        actionState.pushExecutionCondition(hitCondition);
        callAction(getProgramInfo(), actionState, actionType, *actionCall->arguments);
        // Finally, merge in the state of the action call.
        // We can only match if other entries have not previously matched!
        const auto *entryHitCondition =
            new IR::LAnd(hitCondition, new IR::LNot(retProperties.totalHitCondition));
        state.merge(actionState);
        retProperties.totalHitCondition =
            new IR::LOr(retProperties.totalHitCondition, entryHitCondition);
        // We use controlPlaneName() here. TODO: Clean this up?
        retProperties.actionRun = CollapseMux::produceOptimizedMux(
            entryHitCondition,
            new IR::StringLiteral(
                IR::Type_String::get(),
                actionCall->method->checkedTo<IR::PathExpression>()->path->toString()),
            retProperties.actionRun);
    }

    return retProperties;
}

const IR::Expression *TableExecutor::processTable() {
    const auto tableName = getP4Table().controlPlaneName();
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(table, properties);

    // First, execute the default action.
    processDefaultAction();

    // Then, resolve the key.
    const auto *key = getP4Table().getKey();
    if (key == nullptr) {
        auto tableActionList = TableUtils::buildTableActionList(table);
        for (const auto *action : tableActionList) {
            getExecutionState().addReachabilityMapping(action, IR::getBoolLiteral(false));
        }
        const auto *actionPath = TableUtils::getDefaultActionName(getP4Table());
        return new IR::StructExpression(
            nullptr,
            {new IR::NamedExpression("hit", new IR::BoolLiteral(false)),
             new IR::NamedExpression("miss", new IR::BoolLiteral(true)),
             new IR::NamedExpression("action_run", new IR::StringLiteral(IR::Type_String::get(),
                                                                         actionPath->toString())),
             new IR::NamedExpression("table_name",
                                     new IR::StringLiteral(IR::Type_String::get(), tableName))});
    }
    key = resolveKey(key);

    // If the table is immutable, we can not add control-plane entries.
    // We can only execute pre-existing entries.
    if (properties.tableIsImmutable) {
        const auto &retProperties = processConstantTableEntries(key);
        return new IR::StructExpression(
            nullptr,
            {new IR::NamedExpression("hit", retProperties.totalHitCondition),
             new IR::NamedExpression("miss", new IR::LNot(retProperties.totalHitCondition)),
             new IR::NamedExpression("action_run", retProperties.actionRun),
             new IR::NamedExpression("table_name",
                                     new IR::StringLiteral(IR::Type_String::get(), tableName))});
    }

    const auto *tableActionID = ControlPlaneState::getTableActionChoice(tableName);
    // Execute all other possible action options. Get the combination of all possible hits.
    const auto &retProperties = processTableActionOptions(tableActionID, key);
    return new IR::StructExpression(
        nullptr, {new IR::NamedExpression("hit", retProperties.totalHitCondition),
                  new IR::NamedExpression("miss", new IR::LNot(retProperties.totalHitCondition)),
                  new IR::NamedExpression("action_run", retProperties.actionRun),
                  new IR::NamedExpression(
                      "table_name", new IR::StringLiteral(IR::Type_String::get(), tableName))});
}

}  // namespace P4Tools::Flay
