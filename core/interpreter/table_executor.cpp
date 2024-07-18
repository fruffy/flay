#include "backends/p4tools/modules/flay/core/interpreter/table_executor.h"

#include <cstddef>
#include <cstdio>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/target.h"
#include "backends/p4tools/modules/flay/core/lib/simplify_expression.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

namespace {

// Synthesize a list of variables which correspond to a control plane argument.
// We get the unique name of the table coupled with the unique name of the action.
// Getting the unique name is needed to avoid generating duplicate arguments.
IR::Vector<IR::Argument> createActionCallArguments(cstring symbolicTablePrefix, cstring actionName,
                                                   const IR::ParameterList &parameters) {
    IR::Vector<IR::Argument> arguments;
    for (const auto *parameter : parameters.parameters) {
        const IR::Expression *actionArg = nullptr;
        // TODO: This boolean cast hack is only necessary because the P4Info does not contain
        // type information. Is there any way we can simplify this?
        if (parameter->type->is<IR::Type_Boolean>()) {
            actionArg = ControlPlaneState::getTableActionArgument(symbolicTablePrefix, actionName,
                                                                  parameter->controlPlaneName(),
                                                                  IR::Type_Bits::get(1));
            actionArg = new IR::Equ(actionArg, new IR::Constant(IR::Type_Bits::get(1), 1));
        } else {
            actionArg = ControlPlaneState::getTableActionArgument(
                symbolicTablePrefix, actionName, parameter->controlPlaneName(), parameter->type);
        }
        arguments.push_back(new IR::Argument(actionArg));
    }
    return arguments;
}

}  // namespace

TableExecutor::TableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver)
    : _table(table), _resolver(callingResolver) {
    setSymbolicTablePrefix(table.controlPlaneName());
}

ExpressionResolver &TableExecutor::resolver() const { return _resolver; }

void TableExecutor::setSymbolicTablePrefix(cstring name) { _symbolicTablePrefix = name; }

cstring TableExecutor::symbolicTablePrefix() const { return _symbolicTablePrefix; }

const ProgramInfo &TableExecutor::getProgramInfo() const { return resolver().getProgramInfo(); }

ExecutionState &TableExecutor::getExecutionState() const { return resolver().getExecutionState(); }

ControlPlaneConstraints &TableExecutor::controlPlaneConstraints() const {
    return resolver().controlPlaneConstraints();
}

const IR::P4Table &TableExecutor::getP4Table() const { return _table; }

const IR::Key *TableExecutor::resolveKey(const IR::Key *key) const {
    IR::Vector<IR::KeyElement> keyElements;
    bool hasChanged = false;
    for (const auto *keyField : key->keyElements) {
        const auto *expr = keyField->expression;
        bool keyFieldHasChanged = false;
        if (!SymbolicEnv::isSymbolicValue(expr)) {
            expr = resolver().computeResult(expr);
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

KeyMap TableExecutor::computeHitCondition(const IR::Key &key) const {
    KeyMap keyMap;
    for (const auto *keyField : key.keyElements) {
        keyMap.insert(computeTargetMatchType(keyField));
    }
    return keyMap;
}

const TableMatchKey *TableExecutor::computeTargetMatchType(const IR::KeyElement *keyField) const {
    const auto *keyExpr = keyField->expression;
    const auto matchType = keyField->matchType->toString();
    const auto *nameAnnot = keyField->getAnnotation(IR::Annotation::nameAnnotation);
    // Some hidden tables do not have any key name annotations.
    BUG_CHECK(nameAnnot != nullptr /* || properties.tableIsImmutable*/,
              "Non-constant table key without an annotation");
    cstring fieldName;
    if (nameAnnot != nullptr) {
        fieldName = nameAnnot->getName();
    }
    // Create a new variable constant that corresponds to the key expression.
    const auto *ctrlPlaneKey =
        ControlPlaneState::getTableKey(symbolicTablePrefix(), fieldName, keyExpr->type);

    if (matchType == P4Constants::MATCH_KIND_EXACT) {
        return new ExactTableMatchKey(fieldName, ctrlPlaneKey, keyExpr);
    }
    if (matchType == P4Constants::MATCH_KIND_TERNARY) {
        const auto *ternaryMask =
            ControlPlaneState::getTableTernaryMask(symbolicTablePrefix(), fieldName, keyExpr->type);
        return new TernaryTableMatchKey(fieldName, ctrlPlaneKey, ternaryMask, keyExpr);
    }
    if (matchType == P4Constants::MATCH_KIND_LPM) {
        const auto *maskVar = ControlPlaneState::getTableMatchLpmPrefix(symbolicTablePrefix(),
                                                                        fieldName, keyExpr->type);
        return new LpmTableMatchKey(fieldName, ctrlPlaneKey, keyExpr, maskVar);
    }
    P4C_UNIMPLEMENTED("Match type %s not implemented for table keys.", matchType);
}

void TableExecutor::callAction(const ProgramInfo &programInfo,
                               ControlPlaneConstraints &controlPlaneConstraints,
                               ExecutionState &state, const IR::P4Action *actionType,
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
    auto &actionStepper = FlayTarget::getStepper(programInfo, controlPlaneConstraints, state);
    actionType->body->apply(actionStepper);
}

void TableExecutor::processDefaultAction() const {
    auto &state = getExecutionState();
    auto table = getP4Table();
    const auto *defaultAction = getP4Table().getDefaultAction();
    const auto *tableAction = defaultAction->checkedTo<IR::MethodCallExpression>();
    const auto *defaultActionType = state.getP4Action(tableAction);

    // Synthesize arguments for the call based on the action parameters.
    const auto *arguments = tableAction->arguments;
    callAction(getProgramInfo(), controlPlaneConstraints(), state, defaultActionType, *arguments);
}

void TableExecutor::processTableActionOptions(const TableUtils::TableProperties &tableProperties,
                                              const ExecutionState &referenceState,
                                              ReturnProperties &tableReturnProperties) const {
    auto table = getP4Table();
    auto tableActionList = TableUtils::buildTableActionList(table);
    auto &state = getExecutionState();
    const auto *tableActionID = ControlPlaneState::getTableActionChoice(symbolicTablePrefix());
    const auto *tableActive = ControlPlaneState::getTableActive(symbolicTablePrefix());

    for (const auto *action : tableActionList) {
        const auto *actionType =
            state.getP4Action(action->expression->checkedTo<IR::MethodCallExpression>());
        const auto *actionLiteral = IR::StringLiteral::get(actionType->controlPlaneName());

        auto *actionChoice = new IR::LAnd(tableActive, new IR::Equ(tableActionID, actionLiteral));
        tableReturnProperties.totalHitCondition = new IR::LOr(
            tableReturnProperties.totalHitCondition, new IR::Equ(tableActionID, actionLiteral));

        // We use action->controlPlaneName() here, NOT actionType. TODO: Clean this up?
        tableReturnProperties.actionRun = SimplifyExpression::produceSimplifiedMux(
            actionChoice, IR::StringLiteral::get(action->controlPlaneName()),
            tableReturnProperties.actionRun);
        const IR::Expression *actionHitCondition = actionChoice;
        state.addReachabilityMapping(action, actionHitCondition);
        // Synthesize arguments for the call based on the action parameters.
        // If the default action is not immutable, it is possible to change it to any other action
        // present in the table.
        if (!tableProperties.defaultIsImmutable &&
            (action->getAnnotation(IR::Annotation::tableOnlyAnnotation) != nullptr)) {
            actionHitCondition = new IR::LOr(
                actionHitCondition,
                new IR::Equ(ControlPlaneState::getDefaultActionVariable(symbolicTablePrefix()),
                            actionLiteral));
        }
        // We get the control plane name of the action we are calling.
        cstring actionName = actionType->controlPlaneName();
        const auto &parameters = actionType->parameters;
        // IMPORTANT: We clone 'referenceState' here, NOT 'state'.
        auto &actionState = referenceState.clone();
        actionState.pushExecutionCondition(actionHitCondition);
        auto arguments = createActionCallArguments(symbolicTablePrefix(), actionName, *parameters);
        callAction(getProgramInfo(), controlPlaneConstraints(), actionState, actionType, arguments);
        // Finally, merge in the state of the action call.
        state.merge(actionState);
    }
    tableReturnProperties.totalHitCondition =
        new IR::LAnd(tableReturnProperties.totalHitCondition,
                     ControlPlaneState::getTableActive(symbolicTablePrefix()));
}

const IR::Expression *TableExecutor::buildKeyMatches(cstring tablePrefix, const KeyMap &keyMap) {
    if (keyMap.empty()) {
        return IR::BoolLiteral::get(false);
    }
    const IR::Expression *hitCondition = nullptr;
    for (const auto *key : keyMap) {
        const auto *matchExpr = key->computeControlPlaneConstraint();
        if (hitCondition == nullptr) {
            hitCondition = matchExpr;
        } else {
            hitCondition = new IR::LAnd(hitCondition, matchExpr);
        }
    }
    // The table can only "hit" when it is actually configured by the control-plane.
    // Pay attention to how we use "toString" for the path name here.
    // We need to match these choices correctly. TODO: Make this very explicit.
    hitCondition = new IR::LAnd(ControlPlaneState::getTableActive(tablePrefix), hitCondition);
    return hitCondition;
}

const IR::Expression *TableExecutor::processTable() {
    const auto &table = getP4Table();
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(table, properties);

    // Then, resolve the key.
    const auto *key = table.getKey();
    if (key == nullptr) {
        auto tableActionList = TableUtils::buildTableActionList(table);
        for (const auto *action : tableActionList) {
            getExecutionState().addReachabilityMapping(action, IR::BoolLiteral::get(false));
        }
        const auto *actionPath = TableUtils::getDefaultActionName(table);
        return new IR::StructExpression(
            nullptr,
            {new IR::NamedExpression("hit", IR::BoolLiteral::get(false)),
             new IR::NamedExpression("miss", IR::BoolLiteral::get(true)),
             new IR::NamedExpression("action_run", IR::StringLiteral::get(actionPath->toString())),
             new IR::NamedExpression("table_name",
                                     IR::StringLiteral::get(table.controlPlaneName()))});
    }
    key = resolveKey(key);
    const auto *tableKeyExpression =
        buildKeyMatches(symbolicTablePrefix(), computeHitCondition(*key));

    const auto *actionPath = TableUtils::getDefaultActionName(table);
    ReturnProperties tableReturnProperties{IR::BoolLiteral::get(false),
                                           IR::StringLiteral::get(actionPath->path->toString())};

    const auto &referenceState = getExecutionState().clone();

    // First, execute the default action.
    processDefaultAction();

    // Execute all other possible action options. Get the combination of all possible hits.
    processTableActionOptions(properties, referenceState, tableReturnProperties);
    tableReturnProperties.actionRun = SimplifyExpression::produceSimplifiedMux(
        tableReturnProperties.totalHitCondition, tableReturnProperties.actionRun,
        IR::StringLiteral::get(TableUtils::getDefaultActionName(table)->toString()));

    // Add the computed hit expression of the table to its control plane configuration.
    // We substitute this match later with concrete assignments.
    auto tableControlPlaneItem = controlPlaneConstraints().find(table.controlPlaneName());
    if (tableControlPlaneItem != controlPlaneConstraints().end()) {
        auto *tableControlPlaneConfiguration =
            tableControlPlaneItem->second.get().checkedTo<TableConfiguration>();
        tableControlPlaneConfiguration->setTableKeyMatch(
            SimplifyExpression::simplify(tableKeyExpression));
    } else {
        ::error("Table %s has no control plane configuration", table.controlPlaneName());
    }

    return new IR::StructExpression(
        nullptr,
        {new IR::NamedExpression("hit", tableReturnProperties.totalHitCondition),
         new IR::NamedExpression("miss", new IR::LNot(tableReturnProperties.totalHitCondition)),
         new IR::NamedExpression("action_run", tableReturnProperties.actionRun),
         new IR::NamedExpression("table_name", IR::StringLiteral::get(table.controlPlaneName()))});
}

}  // namespace P4Tools::Flay
