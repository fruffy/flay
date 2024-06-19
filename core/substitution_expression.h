#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTION_EXPRESSION_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTION_EXPRESSION_H_

#include <z3++.h>

#include <map>
#include <optional>

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "ir/ir.h"

namespace P4Tools::Flay {

/**************************************************************************************************
SubstitutionExpression
**************************************************************************************************/

struct SubstitutionExpression {
 private:
    /// The conditions for the expression to be executable.
    const IR::Expression *_condition;

    /// The current assigned value to `cond`. If the value is std::nullopt, the result is ambiguous.
    /// Otherwise, cond is considered either true or false and thus always or never true.
    const IR::Expression *_originalExpression;

    std::optional<const IR::Literal *> _substitution;

 public:
    SubstitutionExpression(const IR::Expression *condition,
                           const IR::Expression *originalExpression);

    SubstitutionExpression(const SubstitutionExpression &) = default;
    SubstitutionExpression(SubstitutionExpression &&) = default;
    SubstitutionExpression &operator=(const SubstitutionExpression &) = default;
    SubstitutionExpression &operator=(SubstitutionExpression &&) = default;
    ~SubstitutionExpression() = default;

    /// @returns the original expression.
    [[nodiscard]] const IR::Expression *originalExpression() const;

    /// @returns the current substitution value if it exists.
    [[nodiscard]] std::optional<const IR::Literal *> substitution() const;

    /// @returns the expression which makes this substitution valid.
    [[nodiscard]] const IR::Expression *condition() const;

    /// Assign a concrete reachability value to the expression.
    void setSubstitution(const IR::Literal *substitution);

    /// Unset the substitution.
    void unsetSubstitution();

    /// Update the condition which makes this substitution valid.
    void setCondition(const IR::Expression *cond);
};

/**************************************************************************************************
Z3SubstitutionExpression
**************************************************************************************************/

struct Z3SubstitutionExpression : public SubstitutionExpression {
 private:
    /// The original expression translated to Z3.
    z3::expr _originalZ3Expression;

 public:
    Z3SubstitutionExpression(const IR::Expression *condition,
                             const IR::Expression *originalExpression,
                             z3::expr originalZ3Expression);

    /// @returns the original expression translated to Z3 form.
    [[nodiscard]] const z3::expr &originalZ3Expression() const;
};

/// Maps an expression with validation source information to its value as collected in the Flay
/// analysis.
using ExpressionMap = std::map<const IR::Expression *, SubstitutionExpression *, SourceIdCmp>;

/// The expression map but using Z3 expressions instead of IR expressions.
using Z3ExpressionMap = std::map<const IR::Expression *, Z3SubstitutionExpression *, SourceIdCmp>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SUBSTITUTION_EXPRESSION_H_ */
