#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TARGET_H_

#include <filesystem>
#include <optional>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/targets/v1model/stepper.h"
#include "ir/ir.h"

namespace P4Tools::Flay::V1Model {

class V1ModelFlayTarget : public FlayTarget {
 private:
    V1ModelFlayTarget();

    static const ArchSpec ARCH_SPEC;

 public:
    /// Registers this target.
    static void make();

 protected:
    const ProgramInfo *initProgramImpl(const IR::P4Program *program,
                                       const IR::Declaration_Instance *mainDecl) const override;

    [[nodiscard]] const ArchSpec *getArchSpecImpl() const override;

    [[nodiscard]] FlayStepper &getStepperImpl(const ProgramInfo &programInfo,
                                              ExecutionState &executionState) const override;
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_V1MODEL_TARGET_H_ */
