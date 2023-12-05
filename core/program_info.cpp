#include "backends/p4tools/modules/flay/core/program_info.h"

#include <utility>

#include "lib/exceptions.h"

namespace P4Tools::Flay {

ProgramInfo::ProgramInfo(const IR::P4Program *program) : program(program) {}

/* =============================================================================================
 *  Getters
 * ============================================================================================= */

const std::vector<const IR::Node *> *ProgramInfo::getPipelineSequence() const {
    return &pipelineSequence;
}

const IR::P4Program *ProgramInfo::getProgram() const { return program; }

cstring ProgramInfo::getCanonicalBlockName(cstring programBlockName) const {
    auto it = blockMap.find(programBlockName);
    if (it != blockMap.end()) {
        return it->second;
    }
    BUG("Unable to find var %s in the canonical block map.", programBlockName);
}

}  // namespace P4Tools::Flay
