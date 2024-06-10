
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/table_executor.h"

#include <optional>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay::Tofino {

namespace {

std::optional<const IR::IDeclaration *> checkForActionProfileOrSelector(
    const IR::P4Table &table, const ExecutionState &state) {
    const auto *impl = table.properties->getProperty(cstring("implementation"));
    if (impl == nullptr) {
        return std::nullopt;
    }

    const auto *implExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const IR::IDeclaration *implDeclaration = nullptr;
    const IR::Type_Extern *implTypeDeclaration = nullptr;
    const IR::Vector<IR::Argument> *declarationArguments = nullptr;
    if (const auto *implCall = implExpr->expression->to<IR::ConstructorCallExpression>()) {
        const auto *implDeclarationType = state.resolveType(implCall->constructedType);
        implTypeDeclaration = implDeclarationType->checkedTo<IR::Type_Extern>();
        implDeclaration = implTypeDeclaration;
        declarationArguments = implCall->arguments;
    } else if (const auto *implPath = implExpr->expression->to<IR::PathExpression>()) {
        const auto *declInst = state.findDecl(implPath)->checkedTo<IR::Declaration_Instance>();
        const auto *implDeclarationType = state.resolveType(declInst->type);
        implTypeDeclaration = implDeclarationType->checkedTo<IR::Type_Extern>();
        implDeclaration = declInst;
        declarationArguments = declInst->arguments;
    } else {
        P4C_UNIMPLEMENTED("Unimplemented action profile type %1%.",
                          implExpr->expression->node_type_name());
    }
    if (implTypeDeclaration->name == "ActionProfile") {
        return implDeclaration;
    }

    if (implTypeDeclaration->name == "ActionSelector") {
        constexpr int kexpectedArgumentSize = 5;
        if (declarationArguments->size() < kexpectedArgumentSize) {
            ::error("Action selector %1% requires %2% arguments, but only %3% were provided.",
                    implTypeDeclaration->controlPlaneName(), kexpectedArgumentSize,
                    declarationArguments->size());
            return std::nullopt;
        }
        const auto *actionProfileReference = declarationArguments->at(0)->expression;
        const auto *actionProfileReferencePath = actionProfileReference->to<IR::PathExpression>();
        if (actionProfileReferencePath == nullptr) {
            ::error("Action profile reference %1% is not a path expression.",
                    actionProfileReference);
            return std::nullopt;
        }
        return state.findDecl(actionProfileReferencePath);
    }
    return std::nullopt;
}

}  // namespace

const IR::Expression *Tofino1TableExecutor::computeHitCondition(const IR::Key &key) const {
    const auto *hitCond = TofinoBaseTableExecutor::computeHitCondition(key);
    // TODO: Should we wrap this condition with an "action_profile_configured" symbolic variable?
    return hitCond;
}

Tofino1TableExecutor::Tofino1TableExecutor(const IR::P4Table &table,
                                           ExpressionResolver &callingResolver)
    : TofinoBaseTableExecutor(table, callingResolver) {
    // A table with an action profile/selector implementation shares the entries with other
    // tables.
    // This means that the behavior of these tables must be consistent.
    _actionProfileOrSelector = checkForActionProfileOrSelector(table, getExecutionState());
}

}  // namespace P4Tools::Flay::Tofino
