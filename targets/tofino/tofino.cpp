#include "backends/p4tools/modules/flay/targets/tofino/tofino.h"

#include <cstdlib>

#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/targets/tofino/control_plane_architecture.h"
#include "backends/p4tools/modules/flay/targets/tofino/symbolic_state.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "lib/error.h"

namespace P4Tools::Flay::Tofino {

TofinoBaseCompilerTarget::TofinoBaseCompilerTarget(std::string deviceName, std::string archName)
    : FlayCompilerTarget(std::move(deviceName), std::move(archName)){};

CompilerResultOrError TofinoBaseCompilerTarget::runCompilerImpl(
    const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }
    // Copy the program after the front end.
    auto *originalProgram = program->clone();

    /// After the front end, get the P4Runtime API for the tna architecture.
    /// TODO: We need to implement the P4Runtime handler for Tofino.
    auto *serializer = P4::P4RuntimeSerializer::get();
    serializer->registerArch("tofino",
                             new P4::ControlPlaneAPI::Standard::TofinoArchHandlerBuilder());
    auto p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "tofino");
    if (::errorCount() > 0) {
        return std::nullopt;
    }

    program = runMidEnd(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    // TODO: We only need this because P4Info does not contain information on default actions.
    P4::ReferenceMap refMap;
    program->apply(P4::ResolveReferences(&refMap));

    ASSIGN_OR_RETURN(
        auto initialControlPlaneState,
        TofinoControlPlaneInitializer(refMap).generateInitialControlPlaneConstraints(program),
        std::nullopt);

    return {*new FlayCompilerResult{CompilerResult(*program), *originalProgram, p4runtimeApi,
                                    initialControlPlaneState}};
}

Tofino1CompilerTarget::Tofino1CompilerTarget() : TofinoBaseCompilerTarget("tofino1", "tna") {}

void Tofino1CompilerTarget::make() {
    static TofinoBaseCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Tofino1CompilerTarget();
    }
}

}  // namespace P4Tools::Flay::Tofino
