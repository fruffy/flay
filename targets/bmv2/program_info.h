#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_PROGRAM_INFO_H_

#include <cstddef>
#include <vector>

#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

namespace P4Tools::Flay::V1Model {

class Bmv2V1ModelProgramInfo : public ProgramInfo {
 private:
    /// The program's top level blocks: the parser, the checksum verifier, the MAU pipeline, the
    /// checksum calculator, and the deparser.
    const ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<const IR::Node *> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                     size_t blockIdx) const;

 public:
    Bmv2V1ModelProgramInfo(const FlayCompilerResult &compilerResult,
                           ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

    /// @returns the gress associated with the given parser.
    int getGress(const IR::Type_Declaration *parser) const;

    /// @returns the programmable blocks of the program. Should be 6.
    [[nodiscard]] const ordered_map<cstring, const IR::Type_Declaration *> *getProgrammableBlocks()
        const;

    /// @returns the name of the parameter for a given programmable-block label and the parameter
    /// index. This is the name of the parameter that is used in the P4 program.
    [[nodiscard]] const IR::PathExpression *getBlockParam(cstring blockLabel,
                                                          size_t paramIndex) const;

    [[nodiscard]] const FlayCompilerResult &getCompilerResult() const override;

    DECLARE_TYPEINFO(Bmv2V1ModelProgramInfo);
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_PROGRAM_INFO_H_ */
