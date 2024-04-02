#include "backends/p4tools/modules/flay/targets/bmv2/program_info.h"

#include <map>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace P4Tools::Flay::V1Model {

Bmv2V1ModelProgramInfo::Bmv2V1ModelProgramInfo(
    const FlayCompilerResult &compilerResult,
    ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : ProgramInfo(compilerResult), programmableBlocks(std::move(inputBlocks)) {
    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto *archSpec = FlayTarget::getArchSpec();
    BUG_CHECK(archSpec->getArchVectorSize() == programmableBlocks.size(),
              "The BMV2 architecture requires %1% pipes (provided %2% pipes).",
              archSpec->getArchVectorSize(), programmableBlocks.size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard v1model, this produces
    /// the parser, the checksum verifier, the MAU pipeline, the checksum calculator, and finally
    /// the deparser. This sequence also includes nodes that handle transitions between the
    /// individual component instantiations.
    int pipeIdx = 0;
    for (const auto &declTuple : programmableBlocks) {
        blockMap.emplace(declTuple.second->getName(), declTuple.first);
        // Iterate through the (ordered) pipes of the target architecture.
        // if (declTuple.first == "Ingress") {
        auto subResult = processDeclaration(declTuple.second, pipeIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        // }
        ++pipeIdx;
    }
}

const ordered_map<cstring, const IR::Type_Declaration *> *
Bmv2V1ModelProgramInfo::getProgrammableBlocks() const {
    return &programmableBlocks;
}

std::vector<const IR::Node *> Bmv2V1ModelProgramInfo::processDeclaration(
    const IR::Type_Declaration *typeDecl, size_t /*blockIdx*/) const {
    // Collect parameters.
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    if (applyBlock == nullptr) {
        P4C_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                          typeDecl->node_type_name());
    }
    std::vector<const IR::Node *> cmds;
    // Insert the actual pipeline.
    cmds.emplace_back(typeDecl);

    return cmds;
}

const IR::PathExpression *Bmv2V1ModelProgramInfo::getBlockParam(cstring blockLabel,
                                                                size_t paramIndex) const {
    // Retrieve the block and get the parameter type.
    // TODO: This should be necessary, we should be able to this using only the arch spec.
    // TODO: Make this more general and lift it into program_info core.
    const auto *programmableBlocks = getProgrammableBlocks();
    const auto *typeDecl = programmableBlocks->at(blockLabel);
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    CHECK_NULL(applyBlock);
    const auto *params = applyBlock->getApplyParameters();
    const auto *param = params->getParameter(paramIndex);
    const auto *paramType = param->type;
    // For convenience, resolve type names.
    if (const auto *tn = paramType->to<IR::Type_Name>()) {
        paramType = resolveProgramType(&getP4Program(), tn);
    }

    const auto *archSpec = FlayTarget::getArchSpec();
    auto archIndex = archSpec->getBlockIndex(blockLabel);
    auto archRef = archSpec->getParamName(archIndex, paramIndex);
    return new IR::PathExpression(paramType, new IR::Path(archRef));
}

const FlayCompilerResult &Bmv2V1ModelProgramInfo::getCompilerResult() const {
    return *ProgramInfo::getCompilerResult().checkedTo<FlayCompilerResult>();
}

}  // namespace P4Tools::Flay::V1Model
