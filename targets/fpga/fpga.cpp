#include "backends/p4tools/modules/flay/targets/fpga/fpga.h"

#include <cstdlib>

#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/targets/fpga/symbolic_state.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "lib/error.h"

namespace P4Tools::Flay::Fpga {

FpgaBaseCompilerTarget::FpgaBaseCompilerTarget(std::string deviceName, std::string archName)
    : FlayCompilerTarget(std::move(deviceName), std::move(archName)){};

CompilerResultOrError FpgaBaseCompilerTarget::runCompilerImpl(const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }
    // Copy the program after the front end.
    auto *originalProgram = program->clone();

    /// After the front end, get the P4Runtime API for the pna architecture.
    /// TODO: We need to implement the P4Runtime handler for Fpga.
    auto p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "pna");
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
        FpgaControlPlaneInitializer(refMap).generateInitialControlPlaneConstraints(program),
        std::nullopt);

    return {*new FlayCompilerResult{CompilerResult(*program), *originalProgram, p4runtimeApi,
                                    initialControlPlaneState}};
}

XsaCompilerTarget::XsaCompilerTarget() : FpgaBaseCompilerTarget("xsa", "fpga") {}

void XsaCompilerTarget::make() {
    static FpgaBaseCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new XsaCompilerTarget();
    }
}

}  // namespace P4Tools::Flay::Fpga
