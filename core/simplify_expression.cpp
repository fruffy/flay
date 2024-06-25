#include "backends/p4tools/modules/flay/core/simplify_expression.h"

#include <optional>
#include <utility>

#include "backends/p4tools/modules/flay/core/collapse_dataplane_variables.h"
#include "backends/p4tools/modules/flay/options.h"
#include "frontends/common/constantFolding.h"
#include "ir/irutils.h"
#include "ir/pass_manager.h"
#include "lib/timer.h"

namespace P4Tools {

namespace {

/// Simplifies mux expressions by folding the condition of the Mux down into its branches.
/// For example,
/// "|X(bool)| ? |X(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|"
/// turns into
/// "|X(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
class FoldMuxConditionDown : public Transform {
 private:
    using ExpressionMap = std::map<const IR::Expression *, bool, IR::IsSemanticallyLessComparator>;

    static std::optional<bool> optionalValue(const ExpressionMap &expressionMap,
                                             const IR::Expression *cond) {
        if (const auto *boolLiteral = cond->to<IR::BoolLiteral>()) {
            return boolLiteral->value;
        }
        auto it = expressionMap.find(cond);
        if (it != expressionMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    class FoldExpression : public Transform {
     private:
        ExpressionMap expressionMap;

     public:
        explicit FoldExpression(ExpressionMap expressionMap)
            : expressionMap(std::move(expressionMap)) {}
        FoldExpression() = default;

        const IR::Node *postorder(IR::LAnd *expr) override {
            auto leftOpt = optionalValue(expressionMap, expr->left);
            if (leftOpt.has_value()) {
                if (leftOpt.value()) {
                    auto rightOpt = optionalValue(expressionMap, expr->right);
                    return rightOpt.has_value()
                               ? IR::BoolLiteral::get(rightOpt.value(), expr->getSourceInfo())
                               : expr->right;
                }
                return IR::BoolLiteral::get(false, expr->getSourceInfo());
            }
            auto rightOpt = optionalValue(expressionMap, expr->right);
            if (rightOpt.has_value()) {
                return rightOpt.value() ? expr->left
                                        : IR::BoolLiteral::get(false, expr->getSourceInfo());
            }
            return expr;
        }

        const IR::Node *postorder(IR::LOr *expr) override {
            auto leftOpt = optionalValue(expressionMap, expr->left);
            if (leftOpt.has_value()) {
                if (leftOpt.value()) {
                    return IR::BoolLiteral::get(true, expr->getSourceInfo());
                }
                auto rightOpt = optionalValue(expressionMap, expr->right);
                return rightOpt.has_value()
                           ? IR::BoolLiteral::get(rightOpt.value(), expr->getSourceInfo())
                           : expr->right;
            }
            auto rightOpt = optionalValue(expressionMap, expr->right);
            if (rightOpt.has_value()) {
                return rightOpt.value() ? IR::BoolLiteral::get(true, expr->getSourceInfo())
                                        : expr->left;
            }
            return expr;
        }

        const IR::Node *postorder(IR::LNot *expr) override {
            auto exprIt = expressionMap.find(expr->expr);
            if (exprIt != expressionMap.end()) {
                return IR::BoolLiteral::get(!exprIt->second, expr->getSourceInfo());
            }

            return expr;
        }

        const IR::Node *postorder(IR::Mux *mux) override {
            auto condOpt = optionalValue(expressionMap, mux->e0);
            if (condOpt.has_value()) {
                return condOpt.value() ? mux->e1 : mux->e2;
            }
            if (mux->e1->equiv(*mux->e2)) {
                return mux->e1;
            }
            return mux;
        }
    };

    const IR::Node *preorder(IR::Mux *mux) override {
        // TODO: We should not need to use this prune but it is needed to avoid a massive slowdown
        // for reasons unclear to me.
        prune();
        mux->e0 = mux->e0->apply(FoldExpression());
        mux->e1 = mux->e1->apply(FoldExpression({{mux->e0, true}}));
        mux->e2 = mux->e2->apply(FoldExpression({{mux->e0, false}}));
        return mux;
    }

 public:
    FoldMuxConditionDown() { visitDagOnce = false; }
};

/// Lifts conditions in mux expressions "upward" and converts the conditions into disjunctive or
/// conjunctive expressions.
/// For example,
/// "|X(bool)| ? |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|"
/// turns into
/// "|X(bool)| && |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
/// and
/// "|X(bool)| ? |A(bit<8>)| : |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
/// into
/// "|X(bool)| || |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|".
class LiftMuxConditions : public Transform {
 public:
    LiftMuxConditions() = default;

    const IR::Node *preorder(IR::Mux *mux) override {
        if (const auto *trueMux = mux->e1->to<IR::Mux>()) {
            /// "|X(bool)| ? |Y(bool)| ? |B(bit<8>)| : |A(bit<8>)| : |B(bit<8>)|"
            /// turns into
            /// "|X(bool)| && !|Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
            if (mux->e2->equiv(*trueMux->e1)) {
                mux->e0 = new IR::LAnd(mux->e0, new IR::LNot(trueMux->e0));
                mux->e1 = trueMux->e2;
                return mux;
            }
            /// "|X(bool)| ? |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)| : |B(bit<8>)|"
            /// turns into
            /// "|X(bool)| && |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
            if (mux->e2->equiv(*trueMux->e2)) {
                mux->e0 = new IR::LAnd(mux->e0, trueMux->e0);
                mux->e1 = trueMux->e1;
                return mux;
            }
        }
        if (const auto *falseMux = mux->e2->to<IR::Mux>()) {
            /// "|X(bool)| ? |A(bit<8>)| : |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
            /// turns into
            /// "|X(bool)| || |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
            if (mux->e1->equiv(*falseMux->e1)) {
                mux->e0 = new IR::LOr(mux->e0, falseMux->e0);
                mux->e2 = falseMux->e2;
                return mux;
            }
            /// "|X(bool)| ? |B(bit<8>)| : |Y(bool)| ? |A(bit<8>)| : |B(bit<8>)|"
            /// turns into
            /// "|X(bool)| || !|Y(bool)| ? |B(bit<8>)| : |A(bit<8>)|"
            if (mux->e1->equiv(*falseMux->e2)) {
                mux->e0 = new IR::LOr(mux->e0, new IR::LNot(falseMux->e0));
                mux->e2 = falseMux->e1;
                return mux;
            }
        }
        return mux;
    }

    const IR::Node *postorder(IR::Mux *mux) override {
        if (mux->e1->equiv(*mux->e2)) {
            return mux->e1;
        }
        return mux;
    }
};

class ExpressionStrengthReduction final : public Transform {
 protected:
    /// @returns `true` if @p expr is the constant `1`.
    static bool isOne(const IR::Expression *expr) {
        const auto *cst = expr->to<IR::Constant>();
        if (cst == nullptr) {
            return false;
        }
        return cst->value == 1;
    }
    /// @returns `true` if @p expr is the constant `0`.
    static bool isZero(const IR::Expression *expr) {
        if (const auto *bt = expr->type->to<IR::Type_Bits>()) {
            if (bt->width_bits() == 0) {
                return true;
            }
        }
        const auto *cst = expr->to<IR::Constant>();
        if (cst == nullptr) {
            return false;
        }
        return cst->value == 0;
    }
    /// @returns `true` if @p expr is the constant `true`.
    static bool isTrue(const IR::Expression *expr) {
        const auto *cst = expr->to<IR::BoolLiteral>();
        if (cst == nullptr) {
            return false;
        }
        return cst->value;
    }
    /// @returns `true` if @p expr is the constant `false`.
    static bool isFalse(const IR::Expression *expr) {
        const auto *cst = expr->to<IR::BoolLiteral>();
        if (cst == nullptr) {
            return false;
        }
        return !cst->value;
    }
    /// @returns `true` if @p expr is all ones.
    static bool isAllOnes(const IR::Expression *expr) {
        const auto *cst = expr->to<IR::Constant>();
        if (cst == nullptr) {
            return false;
        }
        big_int value = cst->value;
        if (value <= 0) {
            return false;
        }
        auto bitcnt = bitcount(value);
        return bitcnt == static_cast<uint64_t>(expr->type->width_bits());
    }

 public:
    ExpressionStrengthReduction() {
        visitDagOnce = true;
        setName("StrengthReduction");
    }

    using Transform::postorder;

    const IR::Node *postorder(IR::Cmpl *expr) override {
        if (const auto *a = expr->expr->to<IR::Cmpl>()) {
            return a->expr;
        }
        return expr;
    }
    const IR::Node *postorder(IR::BAnd *expr) override {
        if (isAllOnes(expr->left)) {
            return expr->right;
        }
        if (isAllOnes(expr->right)) {
            return expr->left;
        }
        const auto *l = expr->left->to<IR::Cmpl>();
        const auto *r = expr->right->to<IR::Cmpl>();
        if ((l != nullptr) && (r != nullptr)) {
            return new IR::Cmpl(expr->srcInfo, expr->type,
                                new IR::BOr(expr->srcInfo, expr->type, l->expr, r->expr));
        }

        if (isZero(expr->left)) {
            return expr->left;
        }
        if (isZero(expr->right)) {
            return expr->right;
        }
        if (expr->left->equiv(*expr->right)) {
            return expr->left;
        }
        return expr;
    }

    const IR::Node *postorder(IR::BOr *expr) override {
        if (isZero(expr->left)) {
            return expr->right;
        }
        if (isZero(expr->right)) {
            return expr->left;
        }
        const auto *l = expr->left->to<IR::Cmpl>();
        const auto *r = expr->right->to<IR::Cmpl>();
        if ((l != nullptr) && (r != nullptr)) {
            return new IR::Cmpl(expr->srcInfo, expr->type,
                                new IR::BAnd(expr->srcInfo, expr->type, l->expr, r->expr));
        }
        if (expr->left->equiv(*expr->right)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Equ *expr) override {
        // a == true is the same as a
        if (isTrue(expr->left)) {
            return expr->right;
        }
        if (isTrue(expr->right)) {
            return expr->left;
        }
        // a == false is the same as !a
        if (isFalse(expr->left)) {
            return new IR::LNot(expr->srcInfo, expr->type, expr->right);
        }
        if (isFalse(expr->right)) {
            return new IR::LNot(expr->srcInfo, expr->type, expr->left);
        }
        return expr;
    }
    const IR::Node *postorder(IR::Neq *expr) override {
        // a != true is the same as !a
        if (isTrue(expr->left)) {
            return new IR::LNot(expr->srcInfo, expr->type, expr->right);
        }
        if (isTrue(expr->right)) {
            return new IR::LNot(expr->srcInfo, expr->type, expr->left);
        }
        // a != false is the same as a
        if (isFalse(expr->left)) {
            return expr->right;
        }
        if (isFalse(expr->right)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::BXor *expr) override {
        if (isZero(expr->left)) {
            return expr->right;
        }
        if (isZero(expr->right)) {
            return expr->left;
        }
        bool cmpl = false;
        if (const auto *l = expr->left->to<IR::Cmpl>()) {
            expr->left = l->expr;
            cmpl = !cmpl;
        }
        if (const auto *r = expr->right->to<IR::Cmpl>()) {
            expr->right = r->expr;
            cmpl = !cmpl;
        }
        if (cmpl) {
            return new IR::Cmpl(expr->srcInfo, expr->type, expr);
        }
        if (expr->left->equiv(*expr->right) && (expr->left->type != nullptr) &&
            !expr->left->type->is<IR::Type_Unknown>()) {
            // we assume that this type is right
            return new IR::Constant(expr->srcInfo, expr->left->type, 0);
        }
        return expr;
    }
    const IR::Node *postorder(IR::LAnd *expr) override {
        if (isFalse(expr->left)) {
            return expr->left;
        }
        if (isFalse(expr->right)) {
            return expr->right;
        }
        if (isTrue(expr->left)) {
            return expr->right;
        }
        if (isTrue(expr->right)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::LOr *expr) override {
        if (isTrue(expr->left)) {
            return expr->left;
        }
        if (isTrue(expr->right)) {
            return expr->right;
        }
        if (isFalse(expr->left)) {
            return expr->right;
        }
        if (isFalse(expr->right)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::LNot *expr) override {
        if (const auto *e = expr->expr->to<IR::LNot>()) {
            return e->expr;
        }
        if (const auto *e = expr->expr->to<IR::Equ>()) {
            return new IR::Neq(expr->srcInfo, expr->type, e->left, e->right);
        }
        if (const auto *e = expr->expr->to<IR::Neq>()) {
            return new IR::Equ(expr->srcInfo, expr->type, e->left, e->right);
        }
        if (const auto *e = expr->expr->to<IR::Leq>()) {
            return new IR::Grt(expr->srcInfo, expr->type, e->left, e->right);
        }
        if (const auto *e = expr->expr->to<IR::Geq>()) {
            return new IR::Lss(expr->srcInfo, expr->type, e->left, e->right);
        }
        if (const auto *e = expr->expr->to<IR::Lss>()) {
            return new IR::Geq(expr->srcInfo, expr->type, e->left, e->right);
        }
        if (const auto *e = expr->expr->to<IR::Grt>()) {
            return new IR::Leq(expr->srcInfo, expr->type, e->left, e->right);
        }
        return expr;
    }
    const IR::Node *postorder(IR::Sub *expr) override {
        if (isZero(expr->right)) {
            return expr->left;
        }
        if (isZero(expr->left)) {
            return new IR::Neg(expr->srcInfo, expr->type, expr->right);
        }
        if (expr->left->equiv(*expr->right) && (expr->left->type != nullptr) &&
            !expr->left->type->is<IR::Type_Unknown>()) {
            return new IR::Constant(expr->srcInfo, expr->left->type, 0);
        }
        return expr;
    }
    const IR::Node *postorder(IR::Add *expr) override {
        if (isZero(expr->right)) {
            return expr->left;
        }
        if (isZero(expr->left)) {
            return expr->right;
        }
        return expr;
    }
    const IR::Node *postorder(IR::UPlus *expr) override { return expr->expr; }
    const IR::Node *postorder(IR::Shl *expr) override {
        if (isZero(expr->right)) {
            return expr->left;
        }
        if (const auto *sh2 = expr->left->to<IR::Shl>()) {
            if (sh2->right->type->is<IR::Type_InfInt>() &&
                expr->right->type->is<IR::Type_InfInt>()) {
                // (a << b) << c is a << (b + c)
                auto *result = new IR::Shl(
                    expr->srcInfo, sh2->left->type, sh2->left,
                    new IR::Add(expr->srcInfo, sh2->right->type, sh2->right, expr->right));
                LOG3("Replace " << expr << " with " << result);
                return result;
            }
        }

        if (isZero(expr->left)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Shr *expr) override {
        if (isZero(expr->right)) {
            return expr->left;
        }
        if (const auto *sh2 = expr->left->to<IR::Shr>()) {
            if (sh2->right->type->is<IR::Type_InfInt>() &&
                expr->right->type->is<IR::Type_InfInt>()) {
                // (a >> b) >> c is a >> (b + c)
                auto *result = new IR::Shr(
                    expr->srcInfo, sh2->left->type, sh2->left,
                    new IR::Add(expr->srcInfo, sh2->right->type, sh2->right, expr->right));
                LOG3("Replace " << expr << " with " << result);
                return result;
            }
        }
        if (isZero(expr->left)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Mul *expr) override {
        if (isOne(expr->left)) {
            return expr->right;
        }
        if (isOne(expr->right)) {
            return expr->left;
        }
        if (isZero(expr->left)) {
            return expr->left;
        }
        if (isZero(expr->right)) {
            return expr->right;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Div *expr) override {
        if (isZero(expr->right)) {
            ::error(ErrorType::ERR_EXPRESSION, "%1%: Division by zero", expr);
            return expr;
        }
        if (isOne(expr->right)) {
            return expr->left;
        }

        if (isZero(expr->left)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Mod *expr) override {
        if (isZero(expr->right)) {
            ::error(ErrorType::ERR_EXPRESSION, "%1%: Modulo by zero", expr);
            return expr;
        }
        if (isZero(expr->left)) {
            return expr->left;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Mux *expr) override {
        if (isTrue(expr->e1) && isFalse(expr->e2)) {
            return expr->e0;
        }
        if (isFalse(expr->e1) && isTrue(expr->e2)) {
            return new IR::LNot(expr->srcInfo, expr->type, expr->e0);
        }
        if (const auto *lnot = expr->e0->to<IR::LNot>()) {
            expr->e0 = lnot->expr;
            const auto *tmp = expr->e1;
            expr->e1 = expr->e2;
            expr->e2 = tmp;
            return expr;
        }
        if (expr->e1->equiv(*expr->e2)) {
            return expr->e1;
        }
        return expr;
    }
    const IR::Node *postorder(IR::Mask *mask) override {
        // a &&& 0xFFFF = a
        if (isAllOnes(mask->right)) {
            return mask->left;
        }
        return mask;
    }
    const IR::Node *postorder(IR::Range *range) override {
        // Range a..a is the same as a
        if (const auto *c0 = range->left->to<IR::Constant>()) {
            if (const auto *c1 = range->right->to<IR::Constant>()) {
                if (c0->value == c1->value) {
                    return c0;
                }
            }
        }
        return range;
    }
    const IR::Node *postorder(IR::Concat *expr) override {
        if (const auto *bt = expr->left->type->to<IR::Type_Bits>()) {
            if (bt->width_bits() == 0) {
                return expr->right;
            }
        }
        if (const auto *bt = expr->right->type->to<IR::Type_Bits>()) {
            if (bt->width_bits() == 0) {
                return expr->left;
            }
        }
        return expr;
    }
    const IR::Node *postorder(IR::ArrayIndex *expr) override {
        if (const auto *hse = expr->left->to<IR::HeaderStackExpression>()) {
            if (const auto *cst = expr->right->to<IR::Constant>()) {
                auto index = cst->asInt();
                if (index < 0 || static_cast<size_t>(index) >= hse->components.size()) {
                    ::error(ErrorType::ERR_EXPRESSION, "%1%: Index %2% out of bounds", index, expr);
                    return expr;
                }
                return hse->components.at(index);
            }
        }
        return expr;
    }
};

}  // namespace

namespace SimplifyExpression {

const IR::Expression *produceSimplifiedMux(const IR::Expression *cond,
                                           const IR::Expression *trueExpression,
                                           const IR::Expression *falseExpression) {
    Util::ScopedTimer timer("Mux optimization");
    return simplify(new IR::Mux(trueExpression->type, cond, trueExpression, falseExpression));
}

class ExpressionRewriter : public PassManager {
 public:
    ExpressionRewriter() {
        setName("ExpressionRewriter");
        // Lifted from frontends/p4/optimizeExpression.
        addPasses({
            new PassRepeated({
                new P4::ConstantFolding(nullptr, nullptr, false),
                new ExpressionStrengthReduction(),
                new FoldMuxConditionDown(),
                new LiftMuxConditions(),
            }),
        });
        auto &options = FlayOptions::get();
        if (options.collapseDataPlaneOperations() && !options.skipParsers()) {
            addPasses({new Flay::DataPlaneVariablePropagator()});
        }
    }
};

const IR::Expression *simplify(const IR::Expression *expr) {
    static ExpressionRewriter REWRITER;
    expr = expr->apply(REWRITER);
    BUG_CHECK(::errorCount() == 0, "Encountered errors while trying to simplify expressions.");
    return expr;
}

}  // namespace SimplifyExpression

}  // namespace P4Tools
