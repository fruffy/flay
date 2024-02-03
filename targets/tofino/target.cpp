#include "backends/p4tools/modules/flay/targets/tofino/target.h"

#include <cstddef>
#include <cstdlib>
#include <map>
#include <optional>
#include <vector>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/protobuf/protobuf.h"
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/program_info.h"
#include "backends/p4tools/modules/flay/targets/tofino/tofino1/stepper.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

namespace P4Tools::Flay::Tofino {

/* =============================================================================================
 *  TofinoBaseFlayTarget implementation
 * ============================================================================================= */

TofinoBaseFlayTarget::TofinoBaseFlayTarget(std::string deviceName, std::string archName)
    : FlayTarget(std::move(deviceName), std::move(archName)){};

std::optional<ControlPlaneConstraints> TofinoBaseFlayTarget::computeControlPlaneConstraintsImpl(
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

/* =============================================================================================
 *  Tofino1FlayTarget implementation
 * ============================================================================================= */

Tofino1FlayTarget::Tofino1FlayTarget() : TofinoBaseFlayTarget("tofino1", "tna") {}

void Tofino1FlayTarget::make() {
    static Tofino1FlayTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Tofino1FlayTarget();
    }
}

const ProgramInfo *Tofino1FlayTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    std::vector<const IR::Type_Declaration *> flattenedBlocks;

    for (const auto *arg : *mainDecl->arguments) {
        if (const auto *pathExpr = arg->expression->to<IR::PathExpression>()) {
            // Look up the path expression in the top-level namespace and expect to find a
            // declaration instance.
            const auto *declInstance = findProgramDecl(&compilerResult.getProgram(), pathExpr->path)
                                           ->checkedTo<IR::Declaration_Instance>();
            // Convert declInstance->arguments into a vector of blocks.
            auto blocks =
                argumentsToTypeDeclarations(&compilerResult.getProgram(), declInstance->arguments);
            flattenedBlocks.insert(flattenedBlocks.end(), blocks.begin(), blocks.end());
        }
    }

    // We should have six arguments.
    BUG_CHECK(flattenedBlocks.size() == 6,
              "%1%: The Tofno1 architecture requires 6 pipes. Received %2%.", mainDecl,
              flattenedBlocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    std::map<int, int> declIdToGress;

    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < flattenedBlocks.size(); ++idx) {
        const auto *declType = flattenedBlocks.at(idx);

        auto canonicalName = ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    return new Tofino1ProgramInfo(*compilerResult.checkedTo<FlayCompilerResult>(),
                                  programmableBlocks);
}

const ArchSpec Tofino1FlayTarget::ARCH_SPEC = ArchSpec(
    "Pipeline",
    {
        // parser IngressParserT<H, M>(
        //     packet_in pkt,
        //     out H hdr,
        //     out M ig_md,
        //     @optional out ingress_intrinsic_metadata_t ig_intr_md,
        //     @optional out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm,
        //     @optional out ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr);
        {"IngressParserT",
         {
             nullptr,
             "*hdr",
             "*ig_md",
             "*ig_intr_md",
             "*ig_intr_md_for_tm",
             "*ig_intr_md_from_prsr",
         }},
        // control IngressT<H, M>(
        //     inout H hdr,
        //     inout M ig_md,
        //     @optional in ingress_intrinsic_metadata_t ig_intr_md,
        //     @optional in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
        //     @optional inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        //     @optional inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm);
        {"IngressT",
         {
             "*hdr",
             "*ig_md",
             "*ig_intr_md",
             "*ig_intr_md_from_prsr",
             "*ig_intr_md_for_dprsr",
             "*ig_intr_md_for_tm",
         }},
        // control IngressDeparserT<H, M>(
        //     packet_out pkt,
        //     inout H hdr,
        //     in M metadata,
        //     @optional in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        //     @optional in ingress_intrinsic_metadata_t ig_intr_md);
        {"IngressDeparserT",
         {
             nullptr,
             "*hdr",
             "*ig_md",
             "*ig_intr_md_for_dprsr",
             "*ig_intr_md",
         }},
        // parser EgressParserT<H, M>(
        //     packet_in pkt,
        //     out H hdr,
        //     out M eg_md,
        //     @optional out egress_intrinsic_metadata_t eg_intr_md,
        //     @optional out egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        //     @optional out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr);
        {"EgressParserT",
         {
             nullptr,
             "*hdr",
             "*eg_md",
             "*eg_intr_md",
             "*eg_intr_md_from_prsr",
             "*eg_intr_md_for_dprsr",
         }},
        // control EgressT<H, M>(
        //     inout H hdr,
        //     inout M eg_md,
        //     @optional in egress_intrinsic_metadata_t eg_intr_md,
        //     @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        //     @optional inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        //     @optional inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport);
        {"EgressT",
         {
             "*hdr",
             "*eg_md",
             "*eg_intr_md",
             "*eg_intr_md_from_prsr",
             "*eg_intr_md_for_dprsr",
             "*eg_intr_md_for_oport",
         }},
        // control EgressDeparserT<H, M>(
        //     packet_out pkt,
        //     inout H hdr,
        //     in M metadata,
        //     @optional in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        //     @optional in egress_intrinsic_metadata_t eg_intr_md,
        //     @optional in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr);
        {"EgressDeparserT",
         {
             nullptr,
             "*hdr",
             "*eg_md",
             "*eg_intr_md_for_dprsr",
             "*eg_intr_md",
             "*eg_intr_md_from_prsr",
         }},
    });

const ArchSpec *Tofino1FlayTarget::getArchSpecImpl() const { return &ARCH_SPEC; }

FlayStepper &Tofino1FlayTarget::getStepperImpl(const ProgramInfo &programInfo,
                                               ExecutionState &executionState) const {
    return *new Tofino1FlayStepper(*programInfo.checkedTo<Tofino1ProgramInfo>(), executionState);
}

}  // namespace P4Tools::Flay::Tofino
