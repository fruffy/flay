#include "backends/p4tools/modules/flay/targets/nikss/target.h"

#include <cstddef>
#include <cstdlib>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/flay/core/control_plane/protobuf_utils.h"
#include "backends/p4tools/modules/flay/targets/nikss/symbolic_state.h"
#include "backends/p4tools/modules/flay/targets/nikss/psa/program_info.h"
#include "backends/p4tools/modules/flay/targets/nikss/psa/stepper.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/config/v1/p4info.pb.h"
#pragma GCC diagnostic pop

namespace P4::P4Tools::Flay::Nikss {

using namespace ::P4::literals;

/* =============================================================================================
 *  NikssBaseFlayTarget implementation
 * ============================================================================================= */

NikssBaseFlayTarget::NikssBaseFlayTarget(const std::string &deviceName, const std::string &archName)
    : FlayTarget(deviceName, archName){};

CompilerResultOrError NikssBaseFlayTarget::runCompilerImpl(const CompilerOptions &options,
                                                          const IR::P4Program *program) const {
    program = runFrontend(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }
    /// After the front end, get the P4Runtime API for the pna architecture.
    /// TODO: We need to implement the P4Runtime handler for Nikss.
    std::optional<P4::P4RuntimeAPI> p4runtimeApi;
    auto p4UserInfo = FlayOptions::get().userP4Info();
    if (p4UserInfo.has_value()) {
        ASSIGN_OR_RETURN(
            auto p4Info,
            Protobuf::deserializeObjectFromFile<p4::config::v1::P4Info>(p4UserInfo.value()),
            std::nullopt);
        p4runtimeApi = P4::P4RuntimeAPI(p4Info.New(), nullptr);
    } else {
        /// After the front end, get the P4Runtime API for the V1model architecture.
        p4runtimeApi = P4::P4RuntimeSerializer::get()->generateP4Runtime(program, "pna"_cs);
        if (::P4::errorCount() > 0) {
            return std::nullopt;
        }
    }

    program = runMidEnd(options, program);
    if (program == nullptr) {
        return std::nullopt;
    }
    // Copy the program after the (safe) mid end.
    auto *originalProgram = program->clone();

    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    program = program->apply(mkPrivateMidEnd(options, &refMap, &typeMap));
    ASSIGN_OR_RETURN(auto initialControlPlaneState,
                     NikssControlPlaneInitializer().generateInitialControlPlaneConstraints(program),
                     std::nullopt);

    return {*new FlayCompilerResult{CompilerResult(*program), *originalProgram,
                                    p4runtimeApi.value(), initialControlPlaneState}};
}

/* =============================================================================================
 *  PsaFlayTarget implementation
 * ============================================================================================= */

PsaFlayTarget::PsaFlayTarget() : NikssBaseFlayTarget("nikss", "psa") {}

void PsaFlayTarget::make() {
    static PsaFlayTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new PsaFlayTarget();
    }
}

const ProgramInfo *PsaFlayTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 3, "%1%: The PSA architecture requires 3 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    // Add to Parser, MatchAction, and Deparser.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    return new PsaProgramInfo(*compilerResult.checkedTo<FlayCompilerResult>(), programmableBlocks);
}

const ArchSpec PsaFlayTarget::ARCH_SPEC = ArchSpec(
    "Pipeline"_cs, {
                       // parser Parser<H, M>(packet_in b,
                       //                     out H hdr,
                       //                     inout M meta,
                       //                     inout standard_metadata_t standard_metadata);
                       {"Parser"_cs,
                        {
                            nullptr,
                            "*hdr"_cs,
                            "*meta"_cs,
                            "*standard_metadata"_cs,
                        }},
                       // control MatchAction<H, M>(inout H hdr,
                       //                           inout M meta,
                       //                           inout standard_metadata_t standard_metadata);
                       {"MatchAction"_cs,
                        {
                            "*hdr"_cs,
                            "*meta"_cs,
                            "*standard_metadata"_cs,
                        }},
                       // control Deparser<H, M>(packet_out b,
                       //                        in H hdr,
                       //                        inout M meta,
                       //                        inout standard_metadata_t standard_metadata);
                       {"Deparser"_cs,
                        {
                            nullptr,
                            "*hdr"_cs,
                            "*meta"_cs,
                            "*standard_metadata"_cs,
                        }},
                   });

const ArchSpec *PsaFlayTarget::getArchSpecImpl() const { return &ARCH_SPEC; }

FlayStepper &PsaFlayTarget::getStepperImpl(const ProgramInfo &programInfo,
                                           ControlPlaneConstraints &constraints,
                                           ExecutionState &executionState) const {
    return *new PsaFlayStepper(*programInfo.checkedTo<PsaProgramInfo>(), constraints,
                               executionState);
}

}  // namespace P4::P4Tools::Flay::Nikss
