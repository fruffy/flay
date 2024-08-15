#include "backends/p4tools/modules/flay/core/interpreter/compiler_result.h"

#include <utility>

#include "ir/ir.h"
#include "midend/convertEnums.h"
#include "midend/convertErrors.h"

namespace P4::P4Tools::Flay {

FlayCompilerResult::FlayCompilerResult(CompilerResult compilerResult,
                                       const IR::P4Program &originalProgram,
                                       P4::P4RuntimeAPI p4runtimeApi,
                                       ControlPlaneConstraints defaultControlPlaneConstraints)
    : CompilerResult(std::move(compilerResult)),
      originalProgram(originalProgram),
      p4runtimeApi(p4runtimeApi),
      defaultControlPlaneConstraints(std::move(defaultControlPlaneConstraints)) {}

const IR::P4Program &FlayCompilerResult::getOriginalProgram() const { return originalProgram; }

const P4::P4RuntimeAPI &FlayCompilerResult::getP4RuntimeApi() const { return p4runtimeApi; }

const ControlPlaneConstraints &FlayCompilerResult::getDefaultControlPlaneConstraints() const {
    return defaultControlPlaneConstraints;
}

}  // namespace P4::P4Tools::Flay
