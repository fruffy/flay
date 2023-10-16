#include "backends/p4tools/modules/flay/flay.h"

#include <cstdlib>

#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
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

    const auto &flayOptions = FlayOptions::get();

    SymbolicExecutor symbex(*programInfo);
    symbex.run();
    const auto &executionState = symbex.getExecutionState();

    auto &options = P4CContext::get().options();
    const auto *freshProgram = P4::parseP4File(options);
    if (::errorCount() > 0) {
        return EXIT_FAILURE;
    }

    ElimDeadCode elim(executionState);

    auto &target = FlayTarget::get();

    if (flayOptions.hasControlPlaneConfig()) {
        auto confPath = flayOptions.getControlPlaneConfig();
        if (confPath.extension() == ".proto") {
            MapP4RuntimeIdtoIR idMapper;
            program->apply(idMapper);
            if (::errorCount() > 0) {
                return EXIT_FAILURE;
            }
            auto idToIrMap = idMapper.getP4RuntimeIDtoIRObjectMap();
            auto deserializedConfig = ProtobufDeserializer::deserializeProtobufConfig(confPath);
            auto constraints = ProtobufDeserializer::convertToControlPlaneConstraints(
                deserializedConfig, idToIrMap);
            elim.addControlPlaneConstraints(constraints);
        }
    }

    printf("Checking whether dead code can be removed...\n");
    freshProgram = freshProgram->apply(elim);
    // P4::ToP4 toP4;
    // program->apply(toP4);

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::Flay
