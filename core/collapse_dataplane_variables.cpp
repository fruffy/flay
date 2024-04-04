#include "backends/p4tools/modules/flay/core/collapse_dataplane_variables.h"

namespace P4Tools::Flay {

namespace {

/// Generates a random string.
/// TODO: It would be nice if this was using a UUID generator.
std::string generateRandomString() {
    constexpr int kLength = 20;
    static const char alphaNumericCharacters[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string randomString;
    randomString.reserve(kLength);

    for (int i = 0; i < kLength; ++i) {
        randomString += alphaNumericCharacters[rand() % (sizeof(alphaNumericCharacters) - 1)];
    }

    return randomString;
}

const IR::DataPlaneVariable *getRandomDataPlaneVariable(const IR::Type *type) {
    return new IR::DataPlaneVariable(type, generateRandomString());
}

/// Computes which bits of an expression are sourced from a data-plane variable.
/// These bits are tainted.
static bitvec computeDataPlaneBits(const IR::Expression *expr) {
    CHECK_NULL(expr);
    if (expr->is<IR::SymbolicVariable>()) {
        return {};
    }

    if (expr->is<IR::DataPlaneVariable>()) {
        return {0, static_cast<size_t>(expr->type->width_bits())};
    }

    if (const auto *concatExpr = expr->to<IR::Concat>()) {
        auto lDataPlaneVariableBits = computeDataPlaneBits(concatExpr->left);
        auto rDataPlaneVariableBits = computeDataPlaneBits(concatExpr->right);
        return (lDataPlaneVariableBits << concatExpr->right->type->width_bits()) |
               rDataPlaneVariableBits;
    }
    if (const auto *slice = expr->to<IR::Slice>()) {
        auto subDataPlaneVariableBits = computeDataPlaneBits(slice->e0);
        return subDataPlaneVariableBits.getslice(slice->getL(), slice->type->width_bits());
    }
    if (const auto *binaryExpr = expr->to<IR::Operation_Binary>()) {
        bitvec fullmask(0, expr->type->width_bits());
        if (const auto *shl = binaryExpr->to<IR::Shl>()) {
            if (const auto *shiftConst = shl->right->to<IR::Constant>()) {
                int shift = static_cast<int>(shiftConst->value);
                return fullmask & (computeDataPlaneBits(shl->left) << shift);
            }
            return fullmask;
        }
        if (const auto *shr = binaryExpr->to<IR::Shr>()) {
            if (const auto *shiftConst = shr->right->to<IR::Constant>()) {
                int shift = static_cast<int>(shiftConst->value);
                return computeDataPlaneBits(shr->left) >> shift;
            }
            return fullmask;
        }
        if (binaryExpr->is<IR::BAnd>() || binaryExpr->is<IR::BOr>() || binaryExpr->is<IR::BXor>()) {
            // Bitwise binary operations cannot taint other bits than those tainted in either lhs or
            // rhs.
            return computeDataPlaneBits(binaryExpr->left) | computeDataPlaneBits(binaryExpr->right);
        }
        // Be conservative here. If either of the expressions contain even a single tainted bit, the
        // entire operation is tainted. The reason is that we need to account for overflow. A
        // tainted MSB or LSB can cause an expression to overflow and underflow.
        auto lDataPlaneVariableBits = computeDataPlaneBits(binaryExpr->left);
        auto rDataPlaneVariableBits = computeDataPlaneBits(binaryExpr->right);
        if (lDataPlaneVariableBits.empty() && rDataPlaneVariableBits.empty()) {
            return {};
        }
        return fullmask;
    }
    if (const auto *unaryExpr = expr->to<IR::Operation_Unary>()) {
        return computeDataPlaneBits(unaryExpr->expr);
    }
    if (expr->is<IR::Literal>()) {
        return {};
    }
    if (expr->is<IR::DefaultExpression>()) {
        return {};
    }
    if (const auto *mux = expr->to<IR::Mux>()) {
        return computeDataPlaneBits(mux->e1) & computeDataPlaneBits(mux->e2);
    }
    BUG("Data-plane variable pair collection is unsupported for %1% of type %2%", expr,
        expr->node_type_name());
}

/// Returns true if the expression could be folded into a single data-plane variable.
bool hasDataPlaneVariable(const IR::Expression *expr) {
    if (expr->is<IR::DataPlaneVariable>()) {
        return true;
    }
    if (expr->is<IR::SymbolicVariable>()) {
        return false;
    }

    if (const auto *structExpr = expr->to<IR::StructExpression>()) {
        return std::any_of(structExpr->components.begin(), structExpr->components.end(),
                           [](const IR::NamedExpression *subExpr) {
                               return hasDataPlaneVariable(subExpr->expression);
                           });
    }
    if (const auto *listExpr = expr->to<IR::ListExpression>()) {
        return std::any_of(
            listExpr->components.begin(), listExpr->components.end(),
            [](const IR::Expression *subExpr) { return hasDataPlaneVariable(subExpr); });
    }
    if (const auto *binaryExpr = expr->to<IR::Operation_Binary>()) {
        // We can short-circuit '&&'...
        if (const auto *lAndExpr = binaryExpr->to<IR::LAnd>()) {
            if (const auto *boolVal = lAndExpr->left->to<IR::BoolLiteral>()) {
                if (!boolVal->value) {
                    return false;
                }
            }
        }
        // ...and '||' in some cases.
        if (const auto *lOrExpr = binaryExpr->to<IR::LOr>()) {
            if (const auto *boolVal = lOrExpr->left->to<IR::BoolLiteral>()) {
                if (boolVal->value) {
                    return false;
                }
            }
        }
        return hasDataPlaneVariable(binaryExpr->left) || hasDataPlaneVariable(binaryExpr->right);
    }
    if (const auto *unaryExpr = expr->to<IR::Operation_Unary>()) {
        return hasDataPlaneVariable(unaryExpr->expr);
    }
    if (expr->is<IR::Literal>()) {
        return false;
    }
    if (const auto *slice = expr->to<IR::Slice>()) {
        auto slLeftInt = slice->e1->checkedTo<IR::Constant>()->asInt();
        auto slRightInt = slice->e2->checkedTo<IR::Constant>()->asInt();
        auto dataPlaneBits = computeDataPlaneBits(slice->e0);
        return !(dataPlaneBits & bitvec(slRightInt, slLeftInt - slRightInt + 1)).empty();
    }
    if (const auto *mux = expr->to<IR::Mux>()) {
        return hasDataPlaneVariable(mux->e1) && hasDataPlaneVariable(mux->e2);
    }
    if (expr->is<IR::DefaultExpression>()) {
        return false;
    }
    BUG("Data-plane variable checking is unsupported for %1% of type %2%", expr,
        expr->node_type_name());
}

}  // namespace

DataPlaneVariablePropagator::DataPlaneVariablePropagator() { visitDagOnce = false; }

const IR::Node *DataPlaneVariablePropagator::postorder(IR::Operation_Unary *unary_op) {
    if (hasDataPlaneVariable(unary_op->expr)) {
        return unary_op->expr;
    }
    return unary_op;
}

const IR::Node *DataPlaneVariablePropagator::postorder(IR::Cast *cast) { return cast; }

const IR::Node *DataPlaneVariablePropagator::postorder(IR::Operation_Binary *bin_op) {
    if (hasDataPlaneVariable(bin_op->left)) {
        return bin_op->left;
    }
    if (hasDataPlaneVariable(bin_op->right)) {
        return bin_op->right;
    }
    return bin_op;
}

const IR::Node *DataPlaneVariablePropagator::postorder(IR::LAnd *lAnd) {
    if (lAnd->left->is<IR::DataPlaneVariable>() && lAnd->right->is<IR::DataPlaneVariable>()) {
        return new IR::SymbolicVariable(IR::Type_Boolean::get(), generateRandomString());
    }
    return lAnd;
}

const IR::Node *DataPlaneVariablePropagator::postorder(IR::Operation_Relation *relOp) {
    if (hasDataPlaneVariable(relOp->left) || hasDataPlaneVariable(relOp->right)) {
        return new IR::SymbolicVariable(IR::Type_Boolean::get(), generateRandomString());
    }
    return relOp;
}

const IR::Node *DataPlaneVariablePropagator::postorder(IR::Concat *concat) { return concat; }

const IR::Node *DataPlaneVariablePropagator::postorder(IR::Slice *slice) {
    if (hasDataPlaneVariable(slice)) {
        // We assume a bit type here...
        BUG_CHECK(!slice->e0->is<IR::Type_Bits>(),
                  "Expected Type_Bits for the slice expression but received %1%",
                  slice->e0->type->node_type_name());
        auto slLeftInt = slice->e1->checkedTo<IR::Constant>()->asInt();
        auto slRightInt = slice->e2->checkedTo<IR::Constant>()->asInt();
        auto width = 1 + slLeftInt - slRightInt;
        return getRandomDataPlaneVariable(IR::getBitType(width));
    }
    return slice;
}

const IR::Node *DataPlaneVariablePropagator::preorder(IR::Mux *mux) {
    // TODO: We should not need to use this prune but it is needed to avoid a
    // massive slowdown for reasons unclear to me.
    prune();
    if (hasDataPlaneVariable(mux->e1) && hasDataPlaneVariable(mux->e2)) {
        return getRandomDataPlaneVariable(mux->type);
    }
    return mux;
}

}  // namespace P4Tools::Flay
