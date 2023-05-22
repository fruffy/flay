#include "backends/p4tools/modules/flay/core/program_info.h"

#include <utility>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

ProgramInfo::ProgramInfo(const IR::P4Program *program) : program(program) {}

/* =============================================================================================
 *  Namespaces and declarations
 * ============================================================================================= */

const IR::IDeclaration *ProgramInfo::findProgramDecl(const IR::IGeneralNamespace *ns,
                                                     const IR::Path *path) {
    auto name = path->name.name;
    const auto *decls = ns->getDeclsByName(name)->toVector();
    if (!decls->empty()) {
        // TODO: Figure out what to do with multiple results. Maybe return all of them and
        // let the caller sort it out?
        BUG_CHECK(decls->size() == 1, "Handling of overloaded names not implemented");
        return decls->at(0);
    }
    BUG("Variable %1% not found in the available namespaces.", path);
}

const IR::IDeclaration *ProgramInfo::findProgramDecl(const IR::IGeneralNamespace *ns,
                                                     const IR::PathExpression *pathExpr) {
    return findProgramDecl(ns, pathExpr->path);
}

const IR::Type_Declaration *ProgramInfo::resolveProgramType(const IR::IGeneralNamespace *ns,
                                                            const IR::Type_Name *type) {
    const auto *path = type->path;
    const auto *decl = findProgramDecl(ns, path)->to<IR::Type_Declaration>();
    BUG_CHECK(decl, "Not a type: %1%", path);
    return decl;
}

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
