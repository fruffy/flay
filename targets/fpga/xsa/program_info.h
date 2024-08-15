#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_PROGRAM_INFO_H_

#include <cstddef>
#include <vector>

#include "backends/p4tools/modules/flay/targets/fpga/base/program_info.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

namespace P4::P4Tools::Flay::Fpga {

class XsaProgramInfo : public FpgaBaseProgramInfo {
 private:
    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<const IR::Node *> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                     size_t blockIdx) const final;

 public:
    XsaProgramInfo(const FlayCompilerResult &compilerResult,
                   ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

    DECLARE_TYPEINFO(XsaProgramInfo, FpgaBaseProgramInfo);
};

}  // namespace P4::P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_XSA_PROGRAM_INFO_H_ */
