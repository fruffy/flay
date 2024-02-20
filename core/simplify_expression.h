#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SIMPLIFY_EXPRESSION_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SIMPLIFY_EXPRESSION_H_

#include "ir/ir.h"

namespace P4Tools {

namespace SimplifyExpression {

/// Produce a Mux expression where the amount of sub-expressions has been simplified under the given
/// Mux conditon.
const IR::Expression *produceSimplifiedMux(const IR::Expression *cond,
                                           const IR::Expression *trueExpression,
                                           const IR::Expression *falseExpression);

/// Simplify the given expression using a series of compiler passes.
const IR::Expression *simplify(const IR::Expression *expr);

};  // namespace SimplifyExpression

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_SIMPLIFY_EXPRESSION_H_ */
