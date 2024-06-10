#include "backends/p4tools/modules/flay/core/substitution_expression.h"

#include <utility>

namespace P4Tools::Flay {

/**************************************************************************************************
SubstitutionExpression
**************************************************************************************************/

const IR::Expression *SubstitutionExpression::originalExpression() const {
    return _originalExpression;
}

std::optional<const IR::Literal *> SubstitutionExpression::substitution() const {
    return _substitution;
}
const IR::Expression *SubstitutionExpression::condition() const { return _condition; }

void SubstitutionExpression::setSubstitution(const IR::Literal *substitution) {
    _substitution = substitution;
}

void SubstitutionExpression::unsetSubstitution() { _substitution = std::nullopt; }

void SubstitutionExpression::setCondition(const IR::Expression *cond) { _condition = cond; }

SubstitutionExpression::SubstitutionExpression(const IR::Expression *condition,
                                               const IR::Expression *originalExpression)
    : _condition(condition), _originalExpression(originalExpression) {}

/**************************************************************************************************
Z3SubstitutionExpression
**************************************************************************************************/

Z3SubstitutionExpression::Z3SubstitutionExpression(const IR::Expression *condition,
                                                   const IR::Expression *originalExpression,
                                                   z3::expr originalZ3Expression)
    : SubstitutionExpression(condition, originalExpression),
      _originalZ3Expression(std::move(originalZ3Expression)) {}

const z3::expr &Z3SubstitutionExpression::originalZ3Expression() const {
    return _originalZ3Expression;
}

}  // namespace P4Tools::Flay
