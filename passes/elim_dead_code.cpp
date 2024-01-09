#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <optional>

#include "backends/p4tools/common/lib/table_utils.h"
#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/error.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const P4::ReferenceMap &refMap,
                           const AbstractReachabilityMap &reachabilityMap)
    : reachabilityMap(reachabilityMap), refMap(refMap) {}

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
    const auto *condition = conditionOpt.value();
    auto reachability = condition->getReachability();

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
        const auto *condition = conditionOpt.value();
        auto reachabilityOpt = condition->getReachability();
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

std::optional<const IR::P4Action *> getActionDecl(const P4::ReferenceMap &refMap,
                                                  const IR::Expression &expr) {
    const IR::Path *actionDeclPath = nullptr;
    if (const auto *tableAction = expr.to<IR::MethodCallExpression>()) {
        const auto *actionPath = tableAction->method->to<IR::PathExpression>();
        if (actionPath == nullptr) {
            ::error("Action reference %1% is not a path expression.", tableAction->method);
            return std::nullopt;
        }
        actionDeclPath = actionPath->path;
    } else if (const auto *tableAction = expr.to<IR::PathExpression>()) {
        actionDeclPath = tableAction->path;
    } else {
        ::error("Unsupported action reference %1%.", expr);
        return std::nullopt;
    }
    const auto *actionDecl = refMap.getDeclaration(actionDeclPath, false);
    if (actionDecl == nullptr) {
        ::error("Unable to find action declaration %1%.", actionDeclPath);
        return std::nullopt;
    }
    const auto *action = actionDecl->to<IR::P4Action>();
    if (action == nullptr) {
        ::error("%1% is not an action.", actionDecl);
        return std::nullopt;
    }
    return action;
}

const IR::Node *ElimDeadCode::preorder(IR::MethodCallStatement *stmt) {
    const auto *call = stmt->methodCall->method->to<IR::Member>();
    if (call == nullptr || call->member != IR::IApply::applyMethodName) {
        return stmt;
    }
    const auto *tableReference = call->expr->to<IR::PathExpression>();
    if (tableReference == nullptr) {
        return stmt;
    }

    const auto *tableDecl = refMap.get().getDeclaration(tableReference->path, false);
    if (tableDecl == nullptr) {
        return stmt;
    }
    const auto *table = tableDecl->to<IR::P4Table>();
    if (table == nullptr) {
        return stmt;
    }

    const IR::P4Action *defaultAction = nullptr;

    const auto *defaultActionRef = table->getDefaultAction();
    // If there is no default action set, assume "NoAction".
    if (defaultActionRef != nullptr) {
        auto defaultActionOpt = getActionDecl(refMap, *defaultActionRef);
        if (!defaultActionOpt.has_value()) {
            return stmt;
        }
        defaultAction = defaultActionOpt.value();
    } else {
        // TODO: Shouldn't there be a builtin?
        defaultAction =
            new IR::P4Action("NoAction", new IR::ParameterList(), new IR::BlockStatement());
    }
    auto tableActionList = TableUtils::buildTableActionList(*table);

    IR::IndexedVector<IR::ActionListElement> filteredActionList;
    for (const auto *action : tableActionList) {
        // Do not check the default action.
        auto actionName = action->getName();
        if (actionName == defaultAction->controlPlaneName()) {
            continue;
        }
        auto conditionOpt = reachabilityMap.get().getReachabilityExpression(action);
        if (!conditionOpt.has_value()) {
            ::error(
                "Unable to find node %1% in the reachability map of this execution state. There "
                "might be issues with the source information.",
                action);
            continue;
        }
        const auto *condition = conditionOpt.value();
        auto reachabilityOpt = condition->getReachability();
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

    // There is no action to execute other than an empty action, remove the table.
    if (filteredActionList.size() == 0 && defaultAction->body->components.empty()) {
        ::warning("Removing %1%", stmt);
        return nullptr;
    }

    return stmt;
}

}  // namespace P4Tools::Flay
