#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_REGISTER_H_

#include "backends/p4tools/modules/flay/targets/tofino/target.h"

namespace P4::P4Tools::Flay {

/// Register the Tofino flay targets with the Flay framework.
inline void tofino_registerFlayTarget() { Tofino::Tofino1FlayTarget::make(); }

}  // namespace P4::P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_REGISTER_H_ */
