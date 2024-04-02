#include "backends/p4tools/modules/flay/targets/tofino/base/program_info.h"

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
#include "lib/null.h"

namespace P4Tools::Flay::Tofino {

TofinoBaseProgramInfo::TofinoBaseProgramInfo(
    const FlayCompilerResult &compilerResult,
    ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : ProgramInfo(compilerResult), programmableBlocks(std::move(inputBlocks)) {}

const ordered_map<cstring, const IR::Type_Declaration *> *
TofinoBaseProgramInfo::getProgrammableBlocks() const {
    return &programmableBlocks;
}

const IR::PathExpression *TofinoBaseProgramInfo::getBlockParam(cstring blockLabel,
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

const FlayCompilerResult &TofinoBaseProgramInfo::getCompilerResult() const {
    return *ProgramInfo::getCompilerResult().checkedTo<FlayCompilerResult>();
}

}  // namespace P4Tools::Flay::Tofino
