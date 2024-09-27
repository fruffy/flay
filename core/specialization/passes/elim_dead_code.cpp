#include "backends/p4tools/modules/flay/core/specialization/passes/elim_dead_code.h"

#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/modules/flay/core/lib/return_macros.h"
#include "backends/p4tools/modules/flay/options.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/error.h"

namespace P4::P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const P4::ReferenceMap &refMap,
                           const AbstractReachabilityMap &reachabilityMap)
    : _reachabilityMap(reachabilityMap), _refMap(refMap) {}

const IR::Node *ElimDeadCode::preorder(IR::P4Parser *parser) {
    if (FlayOptions::get().skipParsers()) {
        prune();
    }
    return parser;
}

const IR::Node *ElimDeadCode::preorder(IR::IfStatement *stmt) {
    // Skip if statements within declaration instances for now. These may be registers for example.
    if (findContext<IR::Declaration_Instance>() != nullptr) {
        return stmt;
    }

    auto reachability = _reachabilityMap.get().isNodeReachable(stmt);

    // Ambiguous condition, we can not simplify.
    if (!reachability.has_value()) {
        return stmt;
    }

    if (reachability.value()) {
        printInfo("---DEAD_CODE--- %1% true branch will always be executed.", stmt);
        if (stmt->ifFalse != nullptr) {
            _eliminatedNodes.emplace_back(stmt->ifFalse, nullptr);
        }
        stmt->ifFalse = nullptr;
        return stmt;
    }
    stmt->condition = new IR::LNot(stmt->condition);
    if (stmt->ifFalse != nullptr) {
        printInfo("---DEAD_CODE--- %1% false branch will always be executed.", stmt);
        _eliminatedNodes.emplace_back(stmt->ifTrue, nullptr);
        stmt->ifTrue = stmt->ifFalse;
    } else {
        printInfo("---DEAD_CODE--- %1% true branch can be deleted.", stmt);
        _eliminatedNodes.emplace_back(stmt, nullptr);
        stmt->ifTrue = new IR::EmptyStatement(stmt->getSourceInfo());
    }
    stmt->ifFalse = nullptr;
    return stmt;
}

const IR::Node *ElimDeadCode::preorder(IR::SwitchStatement *switchStmt) {
    IR::Vector<IR::SwitchCase> filteredSwitchCases;
    bool previousFallThrough = false;
    for (const auto *switchCase : switchStmt->cases) {
        if (switchCase->label->is<IR::DefaultExpression>()) {
            filteredSwitchCases.push_back(switchCase);
            break;
        }
        auto isreachabilityOpt = _reachabilityMap.get().isNodeReachable(switchCase);
        if (!isreachabilityOpt.has_value()) {
            printInfo("---DEAD_CODE--- SwitchCase %1% can be executed.", switchCase->label);
            filteredSwitchCases.push_back(switchCase);
            previousFallThrough = switchCase->statement == nullptr;
            continue;
        }
        auto reachability = isreachabilityOpt.value();
        if (reachability) {
            filteredSwitchCases.push_back(switchCase);
            printInfo("---DEAD_CODE--- SwitchCase %1% is always true.", switchCase->label);
            previousFallThrough = switchCase->statement == nullptr;
            if (switchCase->statement != nullptr) {
                break;
            }
            continue;
        }
        printInfo("---DEAD_CODE--- %1% can be deleted.", switchCase->label);
        _eliminatedNodes.emplace_back(switchCase, nullptr);
        // We are removing a statement that had previous fall-through labels.
        if (previousFallThrough && !filteredSwitchCases.empty() &&
            switchCase->statement != nullptr) {
            auto *previous = filteredSwitchCases.back()->clone();
            printInfo("---DEAD_CODE--- Merging statements of %1% into %2%.", switchCase->label,
                      previous->label);
            previous->statement = switchCase->statement;
            filteredSwitchCases.pop_back();
            filteredSwitchCases.push_back(previous);
        }
        previousFallThrough = switchCase->statement == nullptr;
    }
    if (filteredSwitchCases.empty()) {
        return new IR::EmptyStatement(switchStmt->getSourceInfo());
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
            error("Action reference %1% is not a path expression.", tableAction->method);
            return std::nullopt;
        }
        actionDeclPath = actionPath->path;
    } else if (const auto *tableAction = expr.to<IR::PathExpression>()) {
        actionDeclPath = tableAction->path;
    } else {
        error("Unsupported action reference %1%.", expr);
        return std::nullopt;
    }
    const auto *actionDecl = refMap.getDeclaration(actionDeclPath, false);
    if (actionDecl == nullptr) {
        error("Unable to find action declaration %1%.", actionDeclPath);
        return std::nullopt;
    }
    const auto *action = actionDecl->to<IR::P4Action>();
    if (action == nullptr) {
        error("%1% is not an action.", actionDecl);
        return std::nullopt;
    }
    return action;
}

const IR::Node *ElimDeadCode::preorder(IR::Member *member) {
    if (member->member != IR::Type_Table::hit && member->member != IR::Type_Table::miss) {
        return member;
    }

    ASSIGN_OR_RETURN(auto reachability, _reachabilityMap.get().isNodeReachable(member), member);

    const auto *result = IR::BoolLiteral::get(reachability, member->srcInfo);
    printInfo("---DEAD_CODE--- %1% can be replaced with %2%.", member, result->toString());
    _eliminatedNodes.emplace_back(member, nullptr);
    return result;
}

const IR::Node *ElimDeadCode::preorder(IR::MethodCallStatement *stmt) {
    const auto *call = stmt->methodCall->method->to<IR::Member>();
    RETURN_IF_FALSE(call != nullptr && call->member == IR::IApply::applyMethodName, stmt);
    ASSIGN_OR_RETURN(auto &tableReference, call->expr->to<IR::PathExpression>(), stmt);
    ASSIGN_OR_RETURN(auto &tableDecl, _refMap.get().getDeclaration(tableReference.path, false),
                     stmt);
    ASSIGN_OR_RETURN(auto &table, tableDecl.to<IR::P4Table>(), stmt);

    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(table, properties);
    RETURN_IF_FALSE(!properties.tableIsImmutable, stmt);

    // Filter out any actions which are @defaultonly.
    auto tableActionList = TableUtils::buildTableActionList(table);
    ASSIGN_OR_RETURN_WITH_MESSAGE(const auto &defaultAction, table.getDefaultAction(), stmt,
                                  error("Table %1% does not have a default action.", tableDecl));
    ASSIGN_OR_RETURN_WITH_MESSAGE(
        const auto &defaultActionCall, defaultAction.to<IR::MethodCallExpression>(), stmt,
        error("%1% is not a method call expression.", table.getDefaultAction()));
    for (const auto *action : tableActionList) {
        // Do not remove the default action.
        if (defaultActionCall.method->toString() ==
            action->expression->checkedTo<IR::MethodCallExpression>()->method->toString()) {
            continue;
        }
        // We return if a single action is executable for the current table.
        ASSIGN_OR_RETURN(auto reachability, _reachabilityMap.get().isNodeReachable(action), stmt);

        if (reachability) {
            printInfo("---DEAD_CODE--- %1% will always be executed.", action);
            return stmt;
        }
    }

    // Action annotated with `@defaultonly` is ignored before. If no action can be reachable
    // based on previous analysis, we replace the apply call to the default action call.
    // Only when the default action has an empty body, we remove the apply.
    auto decl = getActionDecl(_refMap, defaultActionCall);
    if (decl.has_value() && !decl.value()->body->components.empty()) {
        printInfo("---DEAD_CODE--- Replacing call %1% with default action %2%", stmt,
                  defaultActionCall);
        auto *replacement =
            new IR::MethodCallStatement(defaultActionCall.getSourceInfo(), &defaultActionCall);
        _eliminatedNodes.emplace_back(stmt, replacement);
        return replacement;
    }

    // There is no action to execute other than an empty action, remove the table.
    printInfo("---DEAD_CODE--- Removing %1%", stmt);
    _eliminatedNodes.emplace_back(stmt, nullptr);
    return new IR::EmptyStatement(stmt->getSourceInfo());
}

std::vector<EliminatedReplacedPair> ElimDeadCode::eliminatedNodes() const {
    return _eliminatedNodes;
}

}  // namespace P4::P4Tools::Flay
