#include "backends/p4tools/modules/flay/targets/tofino/tofino1/program_info.h"

#include <map>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/interpreter/target.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4::P4Tools::Flay::Tofino {

Tofino1ProgramInfo::Tofino1ProgramInfo(
    const FlayCompilerResult &compilerResult,
    ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : TofinoBaseProgramInfo(compilerResult, std::move(inputBlocks)) {
    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto *archSpec = FlayTarget::getArchSpec();
    BUG_CHECK(archSpec->getArchVectorSize() == programmableBlocks.size(),
              "The Tofino1 architecture requires %1% pipes (provided %2% pipes).",
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

std::vector<const IR::Node *> Tofino1ProgramInfo::processDeclaration(
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

}  // namespace P4::P4Tools::Flay::Tofino
