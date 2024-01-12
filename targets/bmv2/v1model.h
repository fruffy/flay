#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_V1MODEL_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_V1MODEL_H_

#include "backends/p4tools/modules/flay/core/compiler_target.h"

namespace P4Tools::Flay::V1Model {

class V1ModelCompilerTarget : public FlayCompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    V1ModelCompilerTarget();

    CompilerResultOrError runCompilerImpl(const IR::P4Program *program) const override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_V1MODEL_H_ */
