#include "backends/p4tools/modules/flay/flay.h"

#include <cstdlib>

#include "backends/p4tools/modules/flay/core/symbolic_executor.h"
#include "backends/p4tools/modules/flay/core/target.h"
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

    const auto *programInfo = FlayTarget::initProgram(program);
    if (programInfo == nullptr) {
        ::error("Program not supported by target device and architecture.");
        return EXIT_FAILURE;
    }
    if (::errorCount() > 0) {
        ::error("Flay: Encountered errors during preprocessing. Exiting");
        return EXIT_FAILURE;
    }

    printf("Running analysis...\n");
    SymbolicExecutor symbex(*programInfo);
    symbex.run();
    const auto &executionState = symbex.getExecutionState();

    auto &options = P4CContext::get().options();
    const auto *freshProgram = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }

    const auto &flayOptions = FlayOptions::get();

    printf("Substituting placeholder variables...\n");
    auto &substitutedExecutionState = executionState.substitutePlaceHolders();
    // Initialize the dead code eliminator. Use the Z3Solver for now.
    Z3Solver solver;
    ElimDeadCode elim(substitutedExecutionState, solver);

    // Gather the initial control-plane configuration from a file input, if present.
    if (flayOptions.hasControlPlaneConfig()) {
        printf("Parsing initial control plane configuration...\n");
        auto &target = FlayTarget::get();
        auto constraintsOpt = target.computeControlPlaneConstraints(*program, flayOptions);
        if (constraintsOpt.has_value()) {
            elim.addControlPlaneConstraints(constraintsOpt.value());
        } else {
            return EXIT_FAILURE;
        }
    }

    printf("Checking whether dead code can be removed...\n");
    freshProgram = freshProgram->apply(elim);
    // P4::ToP4 toP4;
    // program->apply(toP4);

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
