#include "backends/p4tools/modules/flay/targets/bmv2/target.h"

#include <cstddef>
#include <cstdlib>
#include <map>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/flay/targets/bmv2/stepper.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "google/protobuf/text_format.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"
#include "p4/config/v1/p4info.pb.h"

namespace P4Tools::Flay::V1Model {

/* =============================================================================================
 *  V1ModelFlayTarget implementation
 * ============================================================================================= */

V1ModelFlayTarget::V1ModelFlayTarget() : FlayTarget("bmv2", "v1model") {}

void V1ModelFlayTarget::make() {
    static V1ModelFlayTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new V1ModelFlayTarget();
    }
}

const ProgramInfo *V1ModelFlayTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 6, "%1%: The BMV2 architecture requires 6 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    std::map<int, int> declIdToGress;

    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    return new Bmv2V1ModelProgramInfo(*compilerResult.checkedTo<FlayCompilerResult>(),
                                      programmableBlocks);
}

const ArchSpec V1ModelFlayTarget::ARCH_SPEC =
    ArchSpec("V1Switch", {// parser Parser<H, M>(packet_in b,
                          //                     out H parsedHdr,
                          //                     inout M meta,
                          //                     inout standard_metadata_t standard_metadata);
                          {"Parser", {nullptr, "*hdr", "*meta", "*standard_metadata"}},
                          // control VerifyChecksum<H, M>(inout H hdr,
                          //                              inout M meta);
                          {"VerifyChecksum", {"*hdr", "*meta"}},
                          // control Ingress<H, M>(inout H hdr,
                          //                       inout M meta,
                          //                       inout standard_metadata_t standard_metadata);
                          {"Ingress", {"*hdr", "*meta", "*standard_metadata"}},
                          // control Egress<H, M>(inout H hdr,
                          //            inout M meta,
                          //            inout standard_metadata_t standard_metadata);
                          {"Egress", {"*hdr", "*meta", "*standard_metadata"}},
                          // control ComputeChecksum<H, M>(inout H hdr,
                          //                       inout M meta);
                          {"ComputeChecksum", {"*hdr", "*meta"}},
                          // control Deparser<H>(packet_out b, in H hdr);
                          {"Deparser", {nullptr, "*hdr"}}});

const ArchSpec *V1ModelFlayTarget::getArchSpecImpl() const { return &ARCH_SPEC; }

FlayStepper &V1ModelFlayTarget::getStepperImpl(const ProgramInfo &programInfo,
                                               ExecutionState &executionState) const {
    return *new V1ModelFlayStepper(*programInfo.checkedTo<Bmv2V1ModelProgramInfo>(),
                                   executionState);
}

std::optional<ControlPlaneConstraints> V1ModelFlayTarget::computeControlPlaneConstraintsImpl(
    const FlayCompilerResult &compilerResult, const FlayOptions &options) const {
    // Initialize some constraints that are active regardless of the control-plane configuration.
    // These constraints can be overridden by the respective control-plane configuration.
    auto constraints = compilerResult.getDefaultControlPlaneConstraints();
    if (!options.hasControlPlaneConfig()) {
        return constraints;
    }
    auto confPath = options.getControlPlaneConfig();
    printInfo("Parsing initial control plane configuration...\n");
    if (confPath.extension() == ".txtpb") {
        auto deserializedConfig =
            ProtobufDeserializer::deserializeProtoObjectFromFile<flaytests::Config>(confPath);
        if (!deserializedConfig.has_value()) {
            return std::nullopt;
        }
        SymbolSet symbolSet;
        if (ProtobufDeserializer::updateControlPlaneConstraints(
                deserializedConfig.value(), *compilerResult.getP4RuntimeApi().p4Info, constraints,
                symbolSet) != EXIT_SUCCESS) {
            return std::nullopt;
        }
        return constraints;
    }

    ::error("Control plane file format %1% not supported for this target.",
            confPath.extension().c_str());
    return std::nullopt;
}

CompilerResultOrError V1ModelFlayTarget::runCompilerImpl(const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }
    // Copy the program after the front end.
    auto *originalProgram = program->clone();

    std::optional<P4::P4RuntimeAPI> p4runtimeApi;
    auto p4UserInfo = FlayOptions::get().userP4Info();
    if (p4UserInfo.has_value()) {
        ASSIGN_OR_RETURN(
            auto p4Info,
            ProtobufDeserializer::deserializeProtoObjectFromFile<p4::config::v1::P4Info>(
                p4UserInfo.value()),
            std::nullopt);
        p4runtimeApi = P4::P4RuntimeAPI(p4Info.New(), nullptr);
    } else {
        /// After the front end, get the P4Runtime API for the V1model architecture.
        p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "v1model");
        if (::errorCount() > 0) {
            return std::nullopt;
        }
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
        Bmv2ControlPlaneInitializer(refMap).generateInitialControlPlaneConstraints(program),
        std::nullopt);

    return {*new FlayCompilerResult{CompilerResult(*program), *originalProgram,
                                    p4runtimeApi.value(), initialControlPlaneState}};
}

}  // namespace P4Tools::Flay::V1Model
