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
#include "lib/error.h"

namespace P4Tools::Flay {

void Flay::registerTarget() {
    // Register all available compiler targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerCompilerTargets();
}

int Flay::mainImpl(const CompilerResult &compilerResult) {
    // Register all available flay targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerFlayTargets();

    // Toggle basic logging information.
    enableInformationLogging();

    // Make sure the input result corresponds to the result we expect.
    const auto *flayCompilerResult = compilerResult.checkedTo<FlayCompilerResult>();
    const auto *programInfo = FlayTarget::produceProgramInfo(compilerResult);
    if (programInfo == nullptr) {
        ::error("Program not supported by target device and architecture.");
        return EXIT_FAILURE;
    }
    if (::errorCount() > 0) {
        ::error("Flay: Encountered errors during preprocessing. Exiting");
        return EXIT_FAILURE;
    }

    printInfo("Running analysis...");
    SymbolicExecutor symbolicExecutor(*programInfo);
    symbolicExecutor.run();

    const auto &executionState = symbolicExecutor.getExecutionState();
    const auto &controlPlaneState = symbolicExecutor.getControlPlaneState();
    auto &options = P4CContext::get().options();
    const auto &flayOptions = FlayOptions::get();

    printInfo("Reparsing original program...");
    const auto *freshProgram = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }

    printInfo("Computing initial control plane constraints...");
    // Gather the initial control-plane configuration. Also from a file input, if present.
    // TODO: Compute this much earlier.
    auto constraintsOpt = P4Tools::Flay::FlayTarget::computeControlPlaneConstraints(
        *flayCompilerResult, flayOptions, controlPlaneState);
    if (!constraintsOpt.has_value()) {
        return EXIT_FAILURE;
    }

    Z3Solver solver;
    auto reachabilityMap = executionState.getReachabilityMap();
    // Initialize the flay service, which includes a dead code eliminator. Use the Z3Solver for now.
    FlayService service(freshProgram, *flayCompilerResult, reachabilityMap, solver,
                        constraintsOpt.value());
    printInfo("Checking whether dead code can be removed with the initial configuration...");
    service.elimControlPlaneDeadCode();

    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }

    if (flayOptions.serverModeActive()) {
        printInfo("Starting flay server...");
        service.startServer(flayOptions.getServerAddress());
    }
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
