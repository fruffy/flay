#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COMPILER_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COMPILER_TARGET_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/modules/flay/control_plane/id_to_ir_map.h"

namespace P4Tools::Flay {

/// Extends the CompilerResult with the associated P4RuntimeApi
class FlayCompilerResult : public CompilerResult {
 private:
    /// The P4RuntimeAPI inferred from this particular  P4 program.
    P4::P4RuntimeAPI p4runtimeApi;

    /// Maps a P4Runtime ID to the corresponding IR node.
    P4RuntimeIdtoIrNodeMap p4RuntimeNodeMap;

 public:
    explicit FlayCompilerResult(CompilerResult compilerResult, P4::P4RuntimeAPI p4runtimeApi,
                                P4RuntimeIdtoIrNodeMap p4RuntimeNodeMap);

    /// @returns the P4RuntimeAPI inferred from this particular BMv2 V1Model P4 program.
    [[nodiscard]] const P4::P4RuntimeAPI &getP4RuntimeApi() const;

    [[nodiscard]] const P4RuntimeIdtoIrNodeMap &getP4RuntimeNodeMap() const;
};

class FlayCompilerTarget : public CompilerTarget {
 protected:
    explicit FlayCompilerTarget(std::string deviceName, std::string archName);

 private:
    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_COMPILER_TARGET_H_ */
