#include "backends/p4tools/modules/flay/flay.h"

#include <cstdlib>

#include "backends/p4tools/modules/flay/core/symbolic_executor.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/lib/logging.h"
#include "backends/p4tools/modules/flay/passes/elim_dead_code.h"
#include "backends/p4tools/modules/flay/register.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "lib/error.h"

namespace P4Tools::Flay {

void Flay::registerTarget() {
    // Register all available compiler targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerCompilerTargets();
}

int Flay::mainImpl(const IR::P4Program *program) {
    // Register all available flay targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerFlayTargets();

    enableInformationLogging();

    const auto *programInfo = FlayTarget::initProgram(program);
    if (programInfo == nullptr) {
        ::error("Program not supported by target device and architecture.");
        return EXIT_FAILURE;
    }
    if (::errorCount() > 0) {
        ::error("Flay: Encountered errors during preprocessing. Exiting");
        return EXIT_FAILURE;
    }

    printInfo("Running analysis...\n");
    SymbolicExecutor symbex(*programInfo);
    symbex.run();
    const auto &executionState = symbex.getExecutionState();
    const auto &controlPlaneState = symbex.getControlPlaneState();

    auto &options = P4CContext::get().options();

    const auto &flayOptions = FlayOptions::get();
    Z3Solver solver;

    /// Substitute any placeholder variables encountered in the execution state.
    printInfo("Substituting placeholder variables...\n");
    auto &substitutedExecutionState = executionState.substitutePlaceholders();

    // Initialize the dead code eliminator. Use the Z3Solver for now.
    ElimDeadCode elim(substitutedExecutionState, solver);

    // Gather the initial control-plane configuration. Also from a file input, if present.
    auto &target = FlayTarget::get();
    auto constraintsOpt =
        target.computeControlPlaneConstraints(*program, flayOptions, controlPlaneState);
    if (!constraintsOpt.has_value()) {
        return EXIT_FAILURE;
    }
    elim.addControlPlaneConstraints(constraintsOpt.value());

    printInfo("Reparsing original program...\n");
    const auto *freshProgram = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }
    printInfo("Checking whether dead code can be removed...\n");
    freshProgram = freshProgram->apply(elim);
    // P4::ToP4 toP4;
    // program->apply(toP4);

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
