#include "backends/p4tools/modules/flay/core/target.h"

#include <string>
#include <utility>

#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

FlayTarget::FlayTarget(std::string deviceName, std::string archName)
    : Target("flay", std::move(deviceName), std::move(archName)) {}

const ProgramInfo *FlayTarget::produceProgramInfoImpl(const CompilerResult &compilerResult) const {
    const auto &program = compilerResult.getProgram();
    // Check that the program has at least one main declaration.
    const auto mainCount = program.getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");

    // Resolve the program's main declaration instance and delegate to the version of
    // produceProgramInfoImpl that takes the main declaration.
    const auto *mainIDecl = program.getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program.main);

    const auto *mainNode = mainIDecl->getNode();
    const auto *mainDecl = mainIDecl->to<IR::Declaration_Instance>();
    BUG_CHECK(mainDecl, "%1%: Program's main declaration is a %2%, not a Declaration_Instance",
              mainNode, mainNode->node_type_name());

    return produceProgramInfoImpl(compilerResult, mainDecl);
}

const FlayTarget &FlayTarget::get() { return Target::get<FlayTarget>("flay"); }

const ArchSpec *FlayTarget::getArchSpec() { return get().getArchSpecImpl(); }

FlayStepper &FlayTarget::getStepper(const ProgramInfo &programInfo,
                                    ExecutionState &executionState) {
    return get().getStepperImpl(programInfo, executionState);
}

std::optional<ControlPlaneConstraints> FlayTarget::computeControlPlaneConstraints(
    const FlayCompilerResult &compilerResult, const FlayOptions &options) {
    return get().computeControlPlaneConstraintsImpl(compilerResult, options);
}

const ProgramInfo *FlayTarget::produceProgramInfo(const CompilerResult &compilerResult) {
    return get().produceProgramInfoImpl(compilerResult);
}

}  // namespace P4Tools::Flay
