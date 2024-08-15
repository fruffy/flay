#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_REGISTER_H_

#include "backends/p4tools/modules/flay/targets/nikss/target.h"

namespace P4::P4Tools::Flay {

/// Register the Nikss flay targets with the Flay framework.
inline void nikss_registerFlayTarget() { Nikss::PsaFlayTarget::make(); }

}  // namespace P4::P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_REGISTER_H_ */
