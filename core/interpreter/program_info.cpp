#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"

#include <utility>

#include "lib/exceptions.h"

namespace P4::P4Tools::Flay {

ProgramInfo::ProgramInfo(const FlayCompilerResult &compilerResult)
    : compilerResult(compilerResult) {}

/* =============================================================================================
 *  Getters
 * ============================================================================================= */

const std::vector<const IR::Node *> *ProgramInfo::getPipelineSequence() const {
    return &pipelineSequence;
}

const FlayCompilerResult &ProgramInfo::getCompilerResult() const { return compilerResult.get(); }

const IR::P4Program &ProgramInfo::getP4Program() const { return getCompilerResult().getProgram(); }

cstring ProgramInfo::getCanonicalBlockName(cstring programBlockName) const {
    auto it = blockMap.find(programBlockName);
    if (it != blockMap.end()) {
        return it->second;
    }
    BUG("Unable to find var %s in the canonical block map.", programBlockName);
}

}  // namespace P4::P4Tools::Flay
