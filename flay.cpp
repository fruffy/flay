#include "backends/p4tools/modules/flay/flay.h"

#include <cstdlib>
#include <string>

#include "backends/p4tools/common/compiler/context.h"
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

int runServer(const FlayOptions &flayOptions, const FlayCompilerResult &flayCompilerResult,
              const ExecutionState &executionState, const ControlPlaneConstraints &constraints) {
    FlayServiceOptions serviceOptions;

    // Initialize the flay service, which includes a dead code eliminator.
    FlayService service(serviceOptions, flayCompilerResult, executionState.getReachabilityMap(),
                        constraints);
    if (::errorCount() > 0) {
        ::error("Encountered errors trying to starting the service.");
        return EXIT_FAILURE;
    }
    printInfo("Starting flay server...");
    service.startServer(flayOptions.getServerAddress());
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

std::optional<const IR::P4Program *> runServiceWrapper(const FlayOptions &flayOptions,
                                                       const FlayCompilerResult &flayCompilerResult,
                                                       const ExecutionState &executionState,
                                                       const ControlPlaneConstraints &constraints) {
    FlayServiceOptions serviceOptions;

    FlayServiceWrapper serviceWrapper(serviceOptions, flayCompilerResult,
                                      executionState.getReachabilityMap(), constraints);
    if (flayOptions.hasConfigurationUpdatePattern()) {
        RETURN_IF_FALSE(serviceWrapper.parseControlUpdatesFromPattern(
                            flayOptions.getConfigurationUpdatePattern()) == EXIT_SUCCESS,
                        std::nullopt);
    }

    RETURN_IF_FALSE(serviceWrapper.run() == EXIT_SUCCESS, std::nullopt);
    return &serviceWrapper.getOptimizedProgram();
}

int Flay::mainImpl(const CompilerResult &compilerResult) {
    // Register all available flay targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerFlayTargets();

    // Make sure the input result corresponds to the result we expect.
    ASSIGN_OR_RETURN_WITH_MESSAGE(const auto &flayCompilerResult,
                                  compilerResult.to<FlayCompilerResult>(), EXIT_FAILURE,
                                  ::error("Expected a FlayCompilerResult."));
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
                     FlayTarget::computeControlPlaneConstraints(flayCompilerResult, flayOptions),
                     EXIT_FAILURE);

    printInfo("Running analysis...");
    SymbolicExecutor symbolicExecutor(*programInfo);
    symbolicExecutor.run();

    printInfo("Starting the service...");
    // If server mode is active, start the server and exit once it has finished.
    if (flayOptions.serverModeActive()) {
        return runServer(flayOptions, flayCompilerResult, symbolicExecutor.getExecutionState(),
                         constraints);
    }

    RETURN_IF_FALSE(runServiceWrapper(flayOptions, flayCompilerResult,
                                      symbolicExecutor.getExecutionState(), constraints),
                    EXIT_FAILURE);
    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

std::optional<const IR::P4Program *> analyseImpl(const std::string &program,
                                                 const CompilerOptions &compilerOptions,
                                                 const FlayOptions &flayOptions) {
    // Register supported compiler targets.
    registerCompilerTargets();

    // Register supported Flay targets.
    registerFlayTargets();

    P4Tools::Target::init(compilerOptions.target.c_str(), compilerOptions.arch.c_str());

    // Set up the compilation context.
    auto *compileContext = new CompileContext<CompilerOptions>();
    compileContext->options() = compilerOptions;
    AutoCompileContext autoContext(compileContext);
    // Run the compiler to get an IR and invoke the tool.
    ASSIGN_OR_RETURN(const auto &compilerResult, P4Tools::CompilerTarget::runCompiler(program),
                     std::nullopt);

    ASSIGN_OR_RETURN_WITH_MESSAGE(const auto &flayCompilerResult,
                                  compilerResult.get().to<FlayCompilerResult>(), std::nullopt,
                                  ::error("Expected a FlayCompilerResult."));

    const auto *programInfo = FlayTarget::produceProgramInfo(flayCompilerResult);
    if (programInfo == nullptr || ::errorCount() > 0) {
        ::error("P4Flay encountered errors during preprocessing.");
        return std::nullopt;
    }
    printInfo("Computing initial control plane constraints...");
    // Gather the initial control-plane configuration. Also from a file input, if present.
    ASSIGN_OR_RETURN(auto constraints,
                     FlayTarget::computeControlPlaneConstraints(flayCompilerResult, flayOptions),
                     std::nullopt);

    printInfo("Running analysis...");
    SymbolicExecutor symbolicExecutor(*programInfo);
    symbolicExecutor.run();

    return runServiceWrapper(flayOptions, flayCompilerResult, symbolicExecutor.getExecutionState(),
                             constraints);
}

std::optional<const IR::P4Program *> Flay::analyse(const std::string &program,
                                                   const CompilerOptions &compilerOptions,
                                                   const FlayOptions &flayOptions) {
    try {
        return analyseImpl(program, compilerOptions, flayOptions);
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace P4Tools::Flay
