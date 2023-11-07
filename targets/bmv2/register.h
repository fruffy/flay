#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_REGISTER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_REGISTER_H_

#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/modules/flay/flay.h"
#include "backends/p4tools/modules/flay/options.h"
#include "backends/p4tools/modules/flay/targets/bmv2/target.h"
#include "backends/p4tools/modules/flay/targets/bmv2/v1model.h"

namespace P4Tools::Flay {

/// Register the V1Model compiler target with the tools framework.
inline void bmv2_registerCompilerTarget() { V1Model::V1ModelCompilerTarget::make(); }

/// Register the V1Model flay target with the Flay framework.
inline void bmv2_registerFlayTarget() { V1Model::V1ModelFlayTarget::make(); }

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_REGISTER_H_ */
