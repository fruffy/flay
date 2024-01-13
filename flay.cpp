#include "backends/p4tools/modules/flay/flay.h"

#include <cstdlib>
#include <string>

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

    const auto &flayOptions = FlayOptions::get();
    printInfo("Computing initial control plane constraints...");
    // Gather the initial control-plane configuration. Also from a file input, if present.
    auto constraintsOpt =
        FlayTarget::computeControlPlaneConstraints(*flayCompilerResult, flayOptions);
    if (!constraintsOpt.has_value()) {
        return EXIT_FAILURE;
    }

    printInfo("Running analysis...");
    SymbolicExecutor symbolicExecutor(*programInfo);
    symbolicExecutor.run();

    const auto &executionState = symbolicExecutor.getExecutionState();
    auto &options = P4CContext::get().options();

    printInfo("Reparsing original program...");
    const auto *freshProgram = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }

    printInfo("Starting the service...");
    FlayServiceOptions serviceOptions;
    // If server mode is active, start the server and exit once it has finished.
    if (flayOptions.serverModeActive()) {
        // Initialize the flay service, which includes a dead code eliminator.
        FlayService service(serviceOptions, freshProgram, *flayCompilerResult,
                            executionState.getReachabilityMap(), constraintsOpt.value());
        printInfo("Starting flay server...");
        service.startServer(flayOptions.getServerAddress());
        return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    FlayServiceWrapper serviceWrapper;
    if (flayOptions.hasConfigurationUpdatePattern()) {
        if (serviceWrapper.parseControlUpdatesFromPattern(
                flayOptions.getConfigurationUpdatePattern()) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    if (serviceWrapper.run(serviceOptions, freshProgram, *flayCompilerResult,
                           executionState.getReachabilityMap(),
                           constraintsOpt.value()) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
