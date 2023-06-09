#include "backends/p4tools/modules/flay/targets/v1model/target.h"

#include <cstddef>
#include <map>
#include <vector>

#include "backends/p4tools/modules/flay/targets/v1model/program_info.h"
#include "backends/p4tools/modules/flay/targets/v1model/stepper.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

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

const ProgramInfo *V1ModelFlayTarget::initProgramImpl(
    const IR::P4Program *program, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    std::vector<const IR::Type_Declaration *> blocks;
    argumentsToTypeDeclarations(program, mainDecl->arguments, blocks);

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

    return new V1ModelProgramInfo(program, programmableBlocks);
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
    return *new V1ModelFlayStepper(*programInfo.checkedTo<V1ModelProgramInfo>(), executionState);
}

}  // namespace P4Tools::Flay::V1Model
