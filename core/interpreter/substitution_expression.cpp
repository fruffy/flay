#include "backends/p4tools/modules/flay/core/interpreter/substitution_expression.h"

#include <utility>

namespace P4::P4Tools::Flay {

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

}  // namespace P4::P4Tools::Flay
