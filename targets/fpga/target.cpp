#include "backends/p4tools/modules/flay/targets/fpga/target.h"

#include <cstddef>
#include <cstdlib>
#include <map>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/targets/fpga/symbolic_state.h"
#include "backends/p4tools/modules/flay/targets/fpga/xsa/program_info.h"
#include "backends/p4tools/modules/flay/targets/fpga/xsa/stepper.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

namespace P4Tools::Flay::Fpga {

/* =============================================================================================
 *  FpgaBaseFlayTarget implementation
 * ============================================================================================= */

FpgaBaseFlayTarget::FpgaBaseFlayTarget(const std::string &deviceName, const std::string &archName)
    : FlayTarget(deviceName, archName){};

std::optional<ControlPlaneConstraints> FpgaBaseFlayTarget::computeControlPlaneConstraintsImpl(
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

CompilerResultOrError FpgaBaseFlayTarget::runCompilerImpl(const IR::P4Program *program) const {
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

/* =============================================================================================
 *  XsaFlayTarget implementation
 * ============================================================================================= */

XsaFlayTarget::XsaFlayTarget() : FpgaBaseFlayTarget("fpga", "xsa") {}

void XsaFlayTarget::make() {
    static XsaFlayTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new XsaFlayTarget();
    }
}

const ProgramInfo *XsaFlayTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 3, "%1%: The XSA architecture requires 3 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    // Add to Parser, MatchAction, and Deparser.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    return new XsaProgramInfo(*compilerResult.checkedTo<FlayCompilerResult>(), programmableBlocks);
}

const ArchSpec XsaFlayTarget::ARCH_SPEC = ArchSpec(
    "Pipeline", {
                    // parser Parser<H, M>(packet_in b,
                    //                     out H hdr,
                    //                     inout M meta,
                    //                     inout standard_metadata_t standard_metadata);
                    {"Parser",
                     {
                         nullptr,
                         "*hdr",
                         "*meta",
                         "*standard_metadata",
                     }},
                    // control MatchAction<H, M>(inout H hdr,
                    //                           inout M meta,
                    //                           inout standard_metadata_t standard_metadata);
                    {"MatchAction",
                     {
                         "*hdr",
                         "*meta",
                         "*standard_metadata",
                     }},
                    // control Deparser<H, M>(packet_out b,
                    //                        in H hdr,
                    //                        inout M meta,
                    //                        inout standard_metadata_t standard_metadata);
                    {"Deparser",
                     {
                         nullptr,
                         "*hdr",
                         "*meta",
                         "*standard_metadata",
                     }},
                });

const ArchSpec *XsaFlayTarget::getArchSpecImpl() const { return &ARCH_SPEC; }

FlayStepper &XsaFlayTarget::getStepperImpl(const ProgramInfo &programInfo,
                                           ExecutionState &executionState) const {
    return *new XsaFlayStepper(*programInfo.checkedTo<XsaProgramInfo>(), executionState);
}

}  // namespace P4Tools::Flay::Fpga
