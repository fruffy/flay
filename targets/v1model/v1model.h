#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_V1MODEL_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_V1MODEL_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"

namespace P4Tools::Flay::V1Model {

class V1ModelCompilerTarget : public CompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;

    V1ModelCompilerTarget();
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_V1MODEL_H_ */
