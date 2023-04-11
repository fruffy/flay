#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PROGRAM_INFO_H_

#include <cstddef>
#include <optional>
#include <vector>

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/castable.h"
#include "midend/coverage.h"

namespace P4Tools::Flay {

/// Stores target-specific information about a P4 program.
class ProgramInfo : public ICastable {
 private:
    /// The P4 program from which this object is derived.
    const IR::P4Program *program;

 protected:
    explicit ProgramInfo(const IR::P4Program *program);

    /// The pipeline sequence of this P4 program. Can be modified by subclasses.
    std::vector<const IR::Node *> pipelineSequence;

 public:
    ProgramInfo(const ProgramInfo &) = default;

    ProgramInfo(ProgramInfo &&) = default;

    ProgramInfo &operator=(const ProgramInfo &) = default;

    ProgramInfo &operator=(ProgramInfo &&) = default;

    ~ProgramInfo() override = default;

    /// @returns the series of nodes that has been computed by this particular target.
    [[nodiscard]] const std::vector<const IR::Node *> *getPipelineSequence() const;

    /// @returns the P4 program associated with this program info.
    [[nodiscard]] const IR::P4Program *getProgram() const;

    /// Looks up a declaration from a path. A BUG occurs if no declaration is found.
    static const IR::IDeclaration *findProgramDecl(const IR::IGeneralNamespace *ns,
                                                   const IR::Path *path);

    /// Looks up a declaration from a path expression. A BUG occurs if no declaration is found.
    static const IR::IDeclaration *findProgramDecl(const IR::IGeneralNamespace *ns,
                                                   const IR::PathExpression *pathExpr);

    /// Resolves a Type_Name in the current environment.
    static const IR::Type_Declaration *resolveProgramType(const IR::IGeneralNamespace *ns,
                                                          const IR::Type_Name *type);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PROGRAM_INFO_H_ */
