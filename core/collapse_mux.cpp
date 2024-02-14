#include "backends/p4tools/modules/flay/core/collapse_mux.h"

#include <optional>
#include <utility>

#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"
#include "ir/irutils.h"
#include "lib/timer.h"

namespace P4Tools {

bool MuxCondComp::operator()(const IR::Expression *s1, const IR::Expression *s2) const {
    return s1->isSemanticallyLess(*s2);
}

std::optional<bool> optionalValue(const ConditionMap &conditionMap, const IR::Expression *cond) {
    if (const auto *boolLiteral = cond->to<IR::BoolLiteral>()) {
        return boolLiteral->value;
    }
    auto it = conditionMap.find(cond);
    if (it != conditionMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

const IR::Node *CollapseMux::preorder(IR::Mux *mux) {
    // TODO: We should not use this prune but it is needed to avoid an infinite loop for an unknown
    // reason.
    prune();
    const auto *cond = mux->e0;
    auto condIt = conditionMap.find(cond);
    if (condIt != conditionMap.end()) {
        if (condIt->second) {
            return mux->e1->apply(CollapseExpression(conditionMap));
        }
        return mux->e2->apply(CollapseExpression(conditionMap));
    }
    mux->e0 = mux->e0->apply(CollapseExpression(conditionMap));
    auto conditionMapE1 = conditionMap;
    conditionMapE1.emplace(cond, true);
    mux->e1 = mux->e1->apply(CollapseExpression(conditionMapE1));
    auto conditionMapE2 = conditionMap;
    conditionMapE2.emplace(cond, false);
    mux->e2 = mux->e2->apply(CollapseExpression(conditionMapE2));
    return mux;
}

const IR::Node *CollapseMux::postorder(IR::Expression *expr) {
    auto it = conditionMap.find(expr);
    if (it != conditionMap.end()) {
        return IR::getBoolLiteral(it->second);
    }
    return expr;
}

const IR::Node *CollapseExpression::postorder(IR::Expression *expr) {
    auto it = conditionMap.find(expr);
    if (it != conditionMap.end()) {
        return IR::getBoolLiteral(it->second);
    }
    return expr;
}

const IR::Node *CollapseExpression::postorder(IR::LAnd *expr) {
    auto leftOpt = optionalValue(conditionMap, expr->left);
    if (leftOpt.has_value()) {
        if (leftOpt.value()) {
            auto rightOpt = optionalValue(conditionMap, expr->right);
            if (rightOpt.has_value()) {
                return IR::getBoolLiteral(rightOpt.value());
            }
            return expr->right;
        }
        return IR::getBoolLiteral(false);
    }
    auto rightOpt = optionalValue(conditionMap, expr->right);
    if (rightOpt.has_value()) {
        if (rightOpt.value()) {
            return expr->left;
        }
        return IR::getBoolLiteral(false);
    }
    return expr;
}

const IR::Node *CollapseExpression::postorder(IR::LOr *expr) {
    auto leftOpt = optionalValue(conditionMap, expr->left);
    if (leftOpt.has_value()) {
        if (leftOpt.value()) {
            return IR::getBoolLiteral(true);
        }
        auto rightOpt = optionalValue(conditionMap, expr->right);
        if (rightOpt.has_value()) {
            return IR::getBoolLiteral(rightOpt.value());
        }
        return expr->right;
    }
    auto rightOpt = optionalValue(conditionMap, expr->right);
    if (rightOpt.has_value()) {
        if (rightOpt.value()) {
            return IR::getBoolLiteral(true);
        }
        return expr->left;
    }
    return expr;
}

const IR::Node *CollapseExpression::postorder(IR::LNot *expr) {
    auto exprIt = conditionMap.find(expr->expr);
    if (exprIt != conditionMap.end()) {
        return IR::getBoolLiteral(!exprIt->second);
    }

    return expr;
}

const IR::Node *CollapseExpression::postorder(IR::Mux *mux) {
    const auto *cond = mux->e0;
    auto condIt = conditionMap.find(cond);
    if (condIt != conditionMap.end()) {
        if (condIt->second) {
            return mux->e1;
        }
        return mux->e2;
    }
    if (const auto *boolCond = cond->to<IR::BoolLiteral>()) {
        return boolCond->value ? mux->e1 : mux->e2;
    }
    return mux;
}

const IR::Expression *CollapseMux::produceOptimizedMux(const IR::Expression *cond,
                                                       const IR::Expression *trueExpression,
                                                       const IR::Expression *falseExpression) {
    Util::ScopedTimer timer("Mux optimization");
    auto *mux = new IR::Mux(trueExpression->type, cond, trueExpression, falseExpression);
    return optimizeExpression(mux->apply(CollapseMux()));
}

const IR::Expression *CollapseMux::optimizeExpression(const IR::Expression *expr) {
    // Lifted from frontends/p4/optimizeExpressions.
    auto pass = PassRepeated({
        new P4::ConstantFolding(nullptr, nullptr, false),
        new P4::StrengthReduction(nullptr, nullptr, nullptr),
        new CollapseMux(),
    });
    expr = expr->apply(pass);
    BUG_CHECK(::errorCount() == 0, "Encountered errors while trying to optimize expressions.");
    return expr;
}

CollapseMux::CollapseMux(const std::map<const IR::Expression *, bool, MuxCondComp> &conditionMap)
    : conditionMap(conditionMap) {
    visitDagOnce = false;
}

}  // namespace P4Tools
