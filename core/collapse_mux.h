#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_

#include "ir/ir.h"

namespace P4Tools {

namespace CollapseMux {

/// Produce a Mux expression where the amount of sub-expressions has been minimized.
const IR::Expression *produceOptimizedMux(const IR::Expression *cond,
                                          const IR::Expression *trueExpression,
                                          const IR::Expression *falseExpression);

const IR::Expression *optimizeExpression(const IR::Expression *expr);

};  // namespace CollapseMux

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COLLAPSE_MUX_H_ */
