#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_REGISTER_H_

#include "backends/p4tools/modules/flay/targets/fpga/target.h"

namespace P4Tools::Flay {

/// Register the Fpga flay targets with the Flay framework.
inline void fpga_registerFlayTarget() { Fpga::XsaFlayTarget::make(); }

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_REGISTER_H_ */
