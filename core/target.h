#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TARGET_H_

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "backends/p4tools/modules/flay/options.h"
#include "ir/ir.h"

namespace P4Tools::Flay {

class FlayTarget : public CompilerTarget {
 public:
    /// @returns the singleton instance for the current target.
    static const FlayTarget &get();

    /// Produces a @ProgramInfo for the given P4 program.
    ///
    /// @returns nullptr if the program is not supported by this target.
    static const ProgramInfo *produceProgramInfo(const CompilerResult &compilerResult);

    /// @returns the stepper that will step through the program, tailored to the target.
    [[nodiscard]] static FlayStepper &getStepper(const ProgramInfo &programInfo,
                                                 ExecutionState &executionState);

    /// A vector that maps the architecture parameters of each pipe to the corresponding
    /// global architecture variables. For example, this map specifies which parameter of each pipe
    /// refers to the input header.
    // The arch map needs to be public to be subclassed.
    /// @returns a reference to the architecture map defined in this target
    static const ArchSpec *getArchSpec();

    /// @returns the initial control plane constraints as computed by the target and the input file.
    static std::optional<ControlPlaneConstraints> computeControlPlaneConstraints(
        const FlayCompilerResult &compilerResult, const FlayOptions &options);

 protected:
    /// @see @produceProgramInfo.
    [[nodiscard]] virtual const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult) const;

    /// @see @getStepper.
    [[nodiscard]] virtual FlayStepper &getStepperImpl(const ProgramInfo &programInfo,
                                                      ExecutionState &executionState) const = 0;

    /// @see @produceProgramInfo.
    virtual const ProgramInfo *produceProgramInfoImpl(
        const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const = 0;

    /// @see @computeControlPlaneConstraints.
    [[nodiscard]] virtual std::optional<ControlPlaneConstraints> computeControlPlaneConstraintsImpl(
        const FlayCompilerResult &compilerResult, const FlayOptions &options) const;

    /// @see getArchSpec
    [[nodiscard]] virtual const ArchSpec *getArchSpecImpl() const = 0;

    explicit FlayTarget(const std::string &deviceName, const std::string &archName);

    virtual PassManager mkPrivateMidEnd(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) const;

    [[nodiscard]] P4::FrontEnd mkFrontEnd() const override;

 private:
    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TARGET_H_ */
