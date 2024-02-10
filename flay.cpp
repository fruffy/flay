#include "backends/p4tools/modules/flay/flay.h"

#include <cstdlib>
#include <string>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/core/symbolic_executor.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/register.h"
#include "backends/p4tools/modules/flay/service/flay_server.h"
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
    ASSIGN_OR_RETURN(auto constraints,
                     FlayTarget::computeControlPlaneConstraints(*flayCompilerResult, flayOptions),
                     EXIT_FAILURE);

    printInfo("Running analysis...");
    SymbolicExecutor symbolicExecutor(*programInfo);
    symbolicExecutor.run();

    const auto &executionState = symbolicExecutor.getExecutionState();

    printInfo("Loading original program...");
    const auto &originalProgram = flayCompilerResult->getOriginalProgram();

    printInfo("Starting the service...");
    FlayServiceOptions serviceOptions;
    // If server mode is active, start the server and exit once it has finished.
    if (flayOptions.serverModeActive()) {
        // Initialize the flay service, which includes a dead code eliminator.
        FlayService service(serviceOptions, originalProgram, *flayCompilerResult,
                            executionState.getReachabilityMap(), constraints);
        if (::errorCount() > 0) {
            ::error("Encountered errors trying to starting the service.");
            return EXIT_FAILURE;
        }
        printInfo("Starting flay server...");
        service.startServer(flayOptions.getServerAddress());
        return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    FlayServiceWrapper serviceWrapper;
    if (flayOptions.hasConfigurationUpdatePattern()) {
        RETURN_IF_FALSE(serviceWrapper.parseControlUpdatesFromPattern(
                            flayOptions.getConfigurationUpdatePattern()) == EXIT_SUCCESS,
                        EXIT_FAILURE);
    }

    RETURN_IF_FALSE(
        serviceWrapper.run(serviceOptions, originalProgram, *flayCompilerResult,
                           executionState.getReachabilityMap(), constraints) == EXIT_SUCCESS,
        EXIT_FAILURE);

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
