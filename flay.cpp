#include "backends/p4tools/modules/flay/flay.h"

#include <google/protobuf/text_format.h>

#include <cstdlib>
#include <string>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/symbolic_executor.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/register.h"
#include "backends/p4tools/modules/flay/service/flay_server.h"
#include "frontends/common/parseInput.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/toP4/toP4.h"
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
    const auto &substitutedExecutionState = executionState.substitutePlaceholders();

    printInfo("Reparsing original program...\n");
    const auto *freshProgram = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }

    // Initialize the flay service, which includes a dead code eliminator. Use the Z3Solver for now.
    // TODO: Get rid of the idMapper.
    MapP4RuntimeIdtoIr idMapper;
    program->apply(idMapper);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }
    FlayService service(freshProgram, substitutedExecutionState, solver,
                        idMapper.getP4RuntimeIdtoIrNodeMap());

    // Gather the initial control-plane configuration. Also from a file input, if present.
    auto constraintsOpt = P4Tools::Flay::FlayTarget::computeControlPlaneConstraints(
        *program, flayOptions, controlPlaneState);
    if (!constraintsOpt.has_value()) {
        return EXIT_FAILURE;
    }
    service.addControlPlaneConstraints(constraintsOpt.value());

    printInfo("Checking whether dead code can be removed...\n");
    service.elimControlPlaneDeadCode();

    if (flayOptions.serverModeActive()) {
        printInfo("Starting flay server...\n");
        service.startServer(flayOptions.getServerAddress());
    }
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
