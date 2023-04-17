#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TARGET_H_

#include <string>
#include <vector>

#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "ir/ir.h"
#include "ir/vector.h"

namespace P4Tools::Flay {

class FlayTarget : public Target {
 public:
    /// @returns the singleton instance for the current target.
    static const FlayTarget &get();

    /// Produces a @ProgramInfo for the given P4 program.
    ///
    /// @returns nullptr if the program is not supported by this target.
    static const ProgramInfo *initProgram(const IR::P4Program *program);

    /// @returns the stepper that will step through the program, tailored to the target.
    [[nodiscard]] static FlayStepper &getStepper(const ProgramInfo &programInfo,
                                                 ExecutionState &executionState);

    /// A vector that maps the architecture parameters of each pipe to the corresponding
    /// global architecture variables. For example, this map specifies which parameter of each pipe
    /// refers to the input header.
    // The arch map needs to be public to be subclassed.
    /// @returns a reference to the architecture map defined in this target
    static const ArchSpec *getArchSpec();

 protected:
    /// @see @initProgram.
    const ProgramInfo *initProgramImpl(const IR::P4Program *program) const;

    // /// @see @getStepper.
    [[nodiscard]] virtual FlayStepper &getStepperImpl(const ProgramInfo &programInfo,
                                                      ExecutionState &executionState) const = 0;

    /// @see @initProgram.
    virtual const ProgramInfo *initProgramImpl(const IR::P4Program *program,
                                               const IR::Declaration_Instance *mainDecl) const = 0;

    /// @see getArchSpec
    [[nodiscard]] virtual const ArchSpec *getArchSpecImpl() const = 0;

    /// Utility function. Converts the list of arguments @inputArgs to a list of type declarations
    ///  and appends the result to @v. Any names appearing in the arguments are
    /// resolved with @ns.
    //
    static void argumentsToTypeDeclarations(const IR::IGeneralNamespace *ns,
                                            const IR::Vector<IR::Argument> *inputArgs,
                                            std::vector<const IR::Type_Declaration *> &resultDecls);

    explicit FlayTarget(std::string deviceName, std::string archName);

 private:
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TARGET_H_ */
