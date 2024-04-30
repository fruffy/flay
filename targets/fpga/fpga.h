#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_FPGA_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_FPGA_H_

#include "backends/p4tools/modules/flay/core/compiler_target.h"

namespace P4Tools::Flay::Fpga {

class FpgaBaseCompilerTarget : public FlayCompilerTarget {
 protected:
    explicit FpgaBaseCompilerTarget(std::string deviceName, std::string archName);

    CompilerResultOrError runCompilerImpl(const IR::P4Program *program) const override;
};

class XsaCompilerTarget : public FpgaBaseCompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    XsaCompilerTarget();
};

}  // namespace P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_FPGA_H_ */
