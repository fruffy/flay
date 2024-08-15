#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_EXPRESSION_STRENGTH_REDUCTION_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_EXPRESSION_STRENGTH_REDUCTION_H_

#include "ir/ir.h"

namespace P4::P4Tools {

class ExpressionStrengthReduction final : public Transform {
 protected:
    /// @returns `true` if @p expr is the constant `1`.
    static bool isOne(const IR::Expression *expr);

    /// @returns `true` if @p expr is the constant `0`.
    static bool isZero(const IR::Expression *expr);

    /// @returns `true` if @p expr is the constant `true`.
    static bool isTrue(const IR::Expression *expr);

    /// @returns `true` if @p expr is the constant `false`.
    static bool isFalse(const IR::Expression *expr);

    /// @returns `true` if @p expr is all ones.
    static bool isAllOnes(const IR::Expression *expr);

 public:
    ExpressionStrengthReduction();

    using Transform::postorder;

    const IR::Node *postorder(IR::Cmpl *expr) override;
    const IR::Node *postorder(IR::BAnd *expr) override;
    const IR::Node *postorder(IR::BOr *expr) override;
    const IR::Node *postorder(IR::Equ *expr) override;
    const IR::Node *postorder(IR::Neq *expr) override;
    const IR::Node *postorder(IR::BXor *expr) override;
    const IR::Node *postorder(IR::LAnd *expr) override;
    const IR::Node *postorder(IR::LOr *expr) override;
    const IR::Node *postorder(IR::LNot *expr) override;
    const IR::Node *postorder(IR::Sub *expr) override;
    const IR::Node *postorder(IR::Add *expr) override;
    const IR::Node *postorder(IR::UPlus *expr) override;
    const IR::Node *postorder(IR::Shl *expr) override;
    const IR::Node *postorder(IR::Shr *expr) override;
    const IR::Node *postorder(IR::Mul *expr) override;
    const IR::Node *postorder(IR::Div *expr) override;
    const IR::Node *postorder(IR::Mod *expr) override;
    const IR::Node *postorder(IR::Mux *expr) override;
    const IR::Node *postorder(IR::Mask *mask) override;
    const IR::Node *postorder(IR::Range *range) override;
    const IR::Node *postorder(IR::Concat *expr) override;
    const IR::Node *postorder(IR::ArrayIndex *expr) override;
};

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_EXPRESSION_STRENGTH_REDUCTION_H_ */
