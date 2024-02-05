#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_REGISTER_H_

#include "backends/p4tools/modules/flay/targets/tofino/target.h"
#include "backends/p4tools/modules/flay/targets/tofino/tofino.h"

namespace P4Tools::Flay {

/// Register the Tofino compiler targets with the tools framework.
inline void tofino_registerCompilerTarget() { Tofino::Tofino1CompilerTarget::make(); }

/// Register the Tofino flay targets with the Flay framework.
inline void tofino_registerFlayTarget() { Tofino::Tofino1FlayTarget::make(); }

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_REGISTER_H_ */
