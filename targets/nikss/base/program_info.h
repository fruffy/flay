#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_PROGRAM_INFO_H_

#include <cstddef>
#include <vector>

#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

namespace P4::P4Tools::Flay::Nikss {

class NikssBaseProgramInfo : public ProgramInfo {
 protected:
    /// The program's top level blocks: the parser, the checksum verifier, the MAU pipeline, the
    /// checksum calculator, and the deparser.
    const ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    virtual std::vector<const IR::Node *> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                             size_t blockIdx) const = 0;

    explicit NikssBaseProgramInfo(const FlayCompilerResult &compilerResult,
                                 ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

 public:
    /// @returns the programmable blocks of the program. Should be 6.
    [[nodiscard]] const ordered_map<cstring, const IR::Type_Declaration *> *getProgrammableBlocks()
        const;

    /// @returns the name of the parameter for a given programmable-block label and the parameter
    /// index. This is the name of the parameter that is used in the P4 program.
    [[nodiscard]] const IR::PathExpression *getBlockParam(cstring blockLabel,
                                                          size_t paramIndex) const;

    [[nodiscard]] const FlayCompilerResult &getCompilerResult() const override;

    DECLARE_TYPEINFO(NikssBaseProgramInfo);
};

}  // namespace P4::P4Tools::Flay::Nikss

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_NIKSS_BASE_PROGRAM_INFO_H_ */
