#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_FLAY_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_FLAY_H_

#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/modules/flay/options.h"
#include "backends/p4tools/modules/flay/service/flay_server.h"

namespace P4Tools::Flay {

/// This is main implementation of the P4Flay tool.
class Flay : public AbstractP4cTool<FlayOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const CompilerResult &compilerResult) override;

 public:
    virtual ~Flay() = default;

    /// Analyse the given program and return an optimized version.
    static std::optional<FlayServiceStatistics> optimizeProgram(
        const std::string &program, const CompilerOptions &compilerOptions,
        const FlayOptions &flayOptions);

    /// Open the program file specified in the compiler options, preprocess it, and return an
    /// optimized version.
    static std::optional<FlayServiceStatistics> optimizeProgram(
        const CompilerOptions &compilerOptions, const FlayOptions &flayOptions);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_FLAY_H_ */
