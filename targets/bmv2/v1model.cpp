#include "backends/p4tools/modules/flay/targets/bmv2/v1model.h"

#include <cstdlib>
#include <string>

#include "backends/p4tools/modules/flay/targets/bmv2/symbolic_state.h"

namespace P4Tools::Flay::V1Model {

V1ModelCompilerTarget::V1ModelCompilerTarget() : FlayCompilerTarget("bmv2", "v1model") {}

void V1ModelCompilerTarget::make() {
    static V1ModelCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new V1ModelCompilerTarget();
    }
}

CompilerResultOrError V1ModelCompilerTarget::runCompilerImpl(const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    /// After the front end, get the P4Runtime API for the V1model architecture.
    auto p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "v1model");

    program = runMidEnd(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    auto &controlPlaneState = *new ProtobufBmv2ControlPlaneState();
    if (controlPlaneState.initializeDefaultState(*p4runtimeApi.p4Info) != EXIT_SUCCESS) {
        return std::nullopt;
    }

    return {*new FlayCompilerResult{CompilerResult(*program), p4runtimeApi, controlPlaneState}};
}

}  // namespace P4Tools::Flay::V1Model
