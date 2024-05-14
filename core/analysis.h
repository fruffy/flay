#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_ANALYSIS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_ANALYSIS_H_

#include "ir/ir.h"

namespace P4Tools::Flay {

size_t computeCyclomaticComplexity(const IR::P4Program &program);

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_ANALYSIS_H_ */
