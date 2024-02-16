#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"

#include <optional>

#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "ir/ir-generated.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/error.h"

namespace P4Tools::Flay {

ElimDeadCode::ElimDeadCode(const P4::ReferenceMap &refMap,
                           const AbstractReachabilityMap &reachabilityMap)
    : reachabilityMap(reachabilityMap), refMap(refMap) {}

const IR::Node *ElimDeadCode::preorder(IR::IfStatement *stmt) {
    // Only analyze statements with valid source location.
    if (!stmt->srcInfo.isValid()) {
        return stmt;
    }

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

    // Ambiguous condition, we can not simplify.
    if (!reachability) {
        return stmt;
    }

    if (reachability.value()) {
        ::warning("%1% true branch will always be executed.", stmt);
        stmt->ifFalse = nullptr;
        eliminatedNodes.push_back(stmt->ifFalse);
        return stmt;
    }
    stmt->condition = new IR::LNot(stmt->condition);
    if (stmt->ifFalse != nullptr) {
        ::warning("%1% false branch will always be executed.", stmt);
        eliminatedNodes.push_back(stmt->ifTrue);
        stmt->ifTrue = stmt->ifFalse;
    } else {
        ::warning("%1% true branch can be deleted.", stmt);
        stmt->ifTrue = new IR::EmptyStatement();
        eliminatedNodes.push_back(stmt);
    }
    stmt->ifFalse = nullptr;
    return stmt;
}

const IR::Node *ElimDeadCode::preorder(IR::SwitchStatement *switchStmt) {
    // Only analyze statements with valid source location.
    if (!switchStmt->srcInfo.isValid()) {
        return switchStmt;
    }

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
        eliminatedNodes.push_back(switchCase);
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

const IR::Node *ElimDeadCode::preorder(IR::Member *member) {
    if (member->member != IR::Type_Table::hit && member->member != IR::Type_Table::miss) {
        return member;
    }
    ASSIGN_OR_RETURN_WITH_MESSAGE(
        const auto *condition, reachabilityMap.get().getReachabilityExpression(member), member,
        ::error("Unable to find node %1% in the reachability map of this execution state. There "
                "might be issues with the source information.",
                member));
    ASSIGN_OR_RETURN(auto reachability, condition->getReachability(), member);

    const auto *result = new IR::BoolLiteral(member->srcInfo, reachability);
    ::warning("%1% can be replaced with %2%.", member, result->toString());
    eliminatedNodes.push_back(member);
    return result;
}

const IR::Node *ElimDeadCode::preorder(IR::MethodCallStatement *stmt) {
    // Only analyze statements with valid source location.
    if (!stmt->srcInfo.isValid()) {
        return stmt;
    }

    const auto *call = stmt->methodCall->method->to<IR::Member>();
    RETURN_IF_FALSE(call != nullptr && call->member == IR::IApply::applyMethodName, stmt);
    ASSIGN_OR_RETURN(auto &tableReference, call->expr->to<IR::PathExpression>(), stmt);
    ASSIGN_OR_RETURN(auto &tableDecl, refMap.get().getDeclaration(tableReference.path, false),
                     stmt);
    ASSIGN_OR_RETURN(auto &table, tableDecl.to<IR::P4Table>(), stmt);

    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(table, properties);
    RETURN_IF_FALSE(!properties.tableIsImmutable, stmt);

    // Filter out any actions which are @defaultonly.
    auto tableActionList = TableUtils::buildTableActionList(table);
    for (const auto *action : tableActionList) {
        ASSIGN_OR_RETURN_WITH_MESSAGE(
            const auto *condition, reachabilityMap.get().getReachabilityExpression(action), stmt,
            ::error(
                "Unable to find node %1% in the reachability map of this execution state. There "
                "might be issues with the source information.",
                action));
        ASSIGN_OR_RETURN(auto reachability, condition->getReachability(), stmt);

        if (reachability) {
            ::warning("%1% will always be executed.", action);
            return stmt;
        }
    }

    // There is no action to execute other than an empty action, remove the table.
    ::warning("Removing %1%", stmt);
    eliminatedNodes.push_back(stmt);
    return new IR::EmptyStatement();
}

std::vector<const IR::Node *> ElimDeadCode::getEliminatedNodes() const { return eliminatedNodes; }

}  // namespace P4Tools::Flay
