#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/modules/flay/control_plane/return_macros.h"
#include "backends/p4tools/modules/flay/options.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/error.h"

namespace P4Tools::Flay {

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
    for (const auto *switchCase : switchStmt->cases) {
        if (switchCase->label->is<IR::DefaultExpression>()) {
            filteredSwitchCases.push_back(switchCase);
            break;
        }
        auto isreachabilityOpt = _reachabilityMap.get().isNodeReachable(switchCase);
        if (!isreachabilityOpt.has_value()) {
            filteredSwitchCases.push_back(switchCase);
            continue;
        }
        auto reachability = isreachabilityOpt.value();
        if (reachability) {
            filteredSwitchCases.push_back(switchCase);
            printInfo("---DEAD_CODE--- %1% is always true.", switchCase->label);
            break;
        }
        printInfo("---DEAD_CODE--- %1% can be deleted.", switchCase->label);
        _eliminatedNodes.emplace_back(switchCase, nullptr);
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
    for (const auto *action : tableActionList) {
        // We return if a single action is executable for the current table.
        auto reachable = _reachabilityMap.get().isNodeReachable(action);
        if (!reachable.has_value()) {
            printInfo("action %1% is ambiguous.", action);
            return stmt;
        }
        printInfo("Checking reachability of %1% : %2%", action,
                  _reachabilityMap.get().isNodeReachable(action) ? "true" : "false");
        ASSIGN_OR_RETURN(auto reachability, _reachabilityMap.get().isNodeReachable(action), stmt);
        if (reachability) {
            printInfo("---DEAD_CODE--- %1% will always be executed.", action);
            return stmt;
        }
    }

    // Action annotated with `@defaultonly` is ignored before. If no action can be reachable
    // based on previous analysis, we replace the apply call to the default action call.
    // Only when the default action has an empty body, we remove the apply.
    const auto *defaultAction = table.getDefaultAction()->to<IR::MethodCallExpression>();
    if (defaultAction != nullptr) {
        auto decl = getActionDecl(_refMap, *defaultAction);
        if (decl.has_value() && !decl.value()->body->components.empty()) {
            printInfo("---DEAD_CODE--- Replacing call %1% with default action %2%", stmt,
                      defaultAction);
            auto *replacement =
                new IR::MethodCallStatement(defaultAction->getSourceInfo(), defaultAction);
            _eliminatedNodes.emplace_back(stmt, replacement);
            return replacement;
        }
    }

    // There is no action to execute other than an empty action, remove the table.
    printInfo("---DEAD_CODE--- Removing %1%", stmt);
    _eliminatedNodes.emplace_back(stmt, nullptr);
    return new IR::EmptyStatement(stmt->getSourceInfo());
}

std::vector<EliminatedReplacedPair> ElimDeadCode::eliminatedNodes() const {
    return _eliminatedNodes;
}

}  // namespace P4Tools::Flay
