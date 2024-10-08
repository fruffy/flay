#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_TARGET_H_

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/core/interpreter/stepper.h"
#include "backends/p4tools/modules/flay/core/interpreter/target.h"
#include "ir/ir.h"

namespace P4::P4Tools::Flay::Nikss {

class NikssBaseFlayTarget : public FlayTarget {
 protected:
    explicit NikssBaseFlayTarget(const std::string &deviceName, const std::string &archName);

    CompilerResultOrError runCompilerImpl(const CompilerOptions &options,
                                          const IR::P4Program *program) const override;
};

class PsaFlayTarget : public NikssBaseFlayTarget {
 private:
    PsaFlayTarget();

    static const ArchSpec ARCH_SPEC;

 public:
    /// Registers this target.
    static void make();

 protected:
    const ProgramInfo *produceProgramInfoImpl(const CompilerResult &compilerResult,
                                              const IR::Declaration_Instance *mainDecl) const final;

    [[nodiscard]] const ArchSpec *getArchSpecImpl() const final;

    [[nodiscard]] FlayStepper &getStepperImpl(const ProgramInfo &programInfo,
                                              ControlPlaneConstraints &constraints,
                                              ExecutionState &executionState) const final;
};

}  // namespace P4::P4Tools::Flay::Nikss

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_TARGET_H_ */
