#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO_H_

#include "backends/p4tools/modules/flay/core/compiler_target.h"

namespace P4Tools::Flay::Tofino {

class TofinoBaseCompilerTarget : public FlayCompilerTarget {
 protected:
    explicit TofinoBaseCompilerTarget(std::string deviceName, std::string archName);

    CompilerResultOrError runCompilerImpl(const IR::P4Program *program) const override;
};

class Tofino1CompilerTarget : public TofinoBaseCompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    Tofino1CompilerTarget();
};

}  // namespace P4Tools::Flay::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_TOFINO_H_ */
