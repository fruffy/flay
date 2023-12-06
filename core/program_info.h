#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PROGRAM_INFO_H_

#include <map>
#include <vector>

#include "ir/ir.h"
#include "ir/node.h"
#include "lib/castable.h"
#include "lib/cstring.h"

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

    /// Maps the programmable blocks in the P4 program to their canonical counterpart.
    std::map<cstring, cstring> blockMap;

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

    /// @returns the canonical name of the program block that is passed in.
    /// Throws a BUG, if the name can not be found.
    [[nodiscard]] cstring getCanonicalBlockName(cstring programBlockName) const;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PROGRAM_INFO_H_ */
