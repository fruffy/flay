#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <optional>

#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/error.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const ReachabilityMap &reachabilityMap)
    : reachabilityMap(reachabilityMap) {}

const IR::Node *ElimDeadCode::preorder(IR::IfStatement *stmt) {
    auto conditionOpt = reachabilityMap.get().getReachabilityExpression(stmt);
    if (!conditionOpt.has_value()) {
        ::error(
            "Unable to find node %1% in the reachability map of this execution state. There might "
            "be "
            "issues with the source information.",
            stmt);
        return stmt;
    }
    auto condition = conditionOpt.value();
    auto reachability = condition.getReachability();

    if (reachability) {
        ::warning("%1% condition can be deleted.", stmt->condition);
        if (reachability.value()) {
            if (stmt->ifFalse != nullptr) {
                ::warning("%1% false branch can be deleted.", stmt->ifFalse);
            }
            return stmt->ifTrue;
        }
        ::warning("%1% true branch can be deleted.", stmt->ifTrue);
        if (stmt->ifFalse != nullptr) {
            return stmt->ifFalse;
        }
        return new IR::EmptyStatement();
    }
    return stmt;
}

const IR::Node *ElimDeadCode::preorder(IR::SwitchStatement *switchStmt) {
    IR::Vector<IR::SwitchCase> filteredSwitchCases;
    for (const auto *switchCase : switchStmt->cases) {
        if (switchCase->label->is<IR::DefaultExpression>()) {
            filteredSwitchCases.push_back(switchCase);
            break;
        }
        auto conditionOpt = reachabilityMap.get().getReachabilityExpression(switchCase);
        if (!conditionOpt.has_value()) {
            ::error(
                "Unable to find node %1% in the reachability map of this execution state. There "
                "might be issues with the source information.",
                switchCase);
            return switchCase;
        }
        auto condition = conditionOpt.value();
        auto reachabilityOpt = condition.getReachability();
        if (!reachabilityOpt.has_value()) {
            filteredSwitchCases.push_back(switchCase);
            continue;
        }
        auto reachability = reachabilityOpt.value();
        if (reachability) {
            filteredSwitchCases.push_back(switchCase);
            ::warning("%1% is always true.", switchCase);
            break;
        }
        ::warning("%1% can be deleted.", switchCase);
    }
    if (filteredSwitchCases.empty()) {
        return new IR::EmptyStatement();
    }
    if (filteredSwitchCases.size() == 1 &&
        filteredSwitchCases[0]->label->is<IR::DefaultExpression>()) {
        return filteredSwitchCases[0]->statement;
    }
    switchStmt->cases = filteredSwitchCases;
    return switchStmt;
}

cstring getActionName(const IR::Expression *methodCallExpr) {
    const auto *tableAction = methodCallExpr->checkedTo<IR::MethodCallExpression>();
    const auto *actionPath = tableAction->method->checkedTo<IR::PathExpression>();
    return actionPath->path->name.name;
}

const IR::Node *ElimDeadCode::preorder(IR::P4Table *table) {
    IR::IndexedVector<IR::ActionListElement> filteredActionList;
    const auto *defaultAction = table->getDefaultAction();
    for (const auto *action : table->getActionList()->actionList) {
        auto conditionOpt = reachabilityMap.get().getReachabilityExpression(action);
        if (!conditionOpt.has_value()) {
            ::error(
                "Unable to find node %1% in the reachability map of this execution state. There "
                "might be issues with the source information.",
                action);
        }
        auto condition = conditionOpt.value();
        auto reachabilityOpt = condition.getReachability();
        if (!reachabilityOpt.has_value()) {
            filteredActionList.push_back(action);
            continue;
        }
        auto reachability = reachabilityOpt.value();
        if (reachability) {
            filteredActionList.push_back(action);
            ::warning("%1% will always be executed.", action);
            break;
        }
    }

    auto *properties = table->properties->clone();
    auto &propertyVector = properties->properties;
    for (const auto it = propertyVector.begin(); it != propertyVector.end();) {
        const auto *property = *it;
        if (property->is<IR::Property>() &&
            property->to<IR::Property>()->name.name == IR::TableProperties::actionsPropertyName) {
            propertyVector.erase(it);
        }
    }
    propertyVector.push_back(new IR::Property(IR::TableProperties::actionsPropertyName,
                                              new IR::ActionList(filteredActionList), false));
    table->properties = properties;
    return table;
}

}  // namespace P4Tools::Flay
