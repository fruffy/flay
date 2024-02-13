#include "backends/p4tools/modules/flay/core/compiler_target.h"

#include <utility>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/convert_varbits.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "midend/booleanKeys.h"
#include "midend/convertEnums.h"
#include "midend/convertErrors.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateTypedefs.h"
#include "midend/hsIndexSimplify.h"
#include "midend/local_copyprop.h"
#include "midend/orderArguments.h"
#include "midend/parserUnroll.h"
#include "midend/removeExits.h"
#include "midend/removeLeftSlices.h"
#include "midend/simplifySelectList.h"

namespace P4Tools::Flay {

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

FlayCompilerTarget::FlayCompilerTarget(std::string deviceName, std::string archName)
    : CompilerTarget(std::move(deviceName), std::move(archName)) {}

/// Implements the default enum-conversion policy, which converts all enums to bit<32>.
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum * /*type*/) const override { return true; }

    [[nodiscard]] unsigned enumSize(unsigned /*enumCount*/) const override { return 32; }
};

class ErrorOn32Bits : public P4::ChooseErrorRepresentation {
    bool convert(const IR::Type_Error * /*type*/) const override { return true; }

    [[nodiscard]] unsigned errorSize(unsigned /*errorCount*/) const override { return 32; }
};

MidEnd FlayCompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    midEnd.setStopOnError(true);
    auto *refMap = midEnd.getRefMap();
    auto *typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Remove exit statements from the program.
        // TODO: We should not depend on this pass. It has bugs.
        new P4::RemoveExits(refMap, typeMap),
        // Replace types introduced by 'type' with 'typedef'.
        new P4::EliminateNewtype(refMap, typeMap),
        // Replace serializable enum constants with their values.
        new P4::EliminateSerEnums(refMap, typeMap),
        // Make sure that we have no TypeDef left in the program.
        new P4::EliminateTypedef(refMap, typeMap),
        // Sort call arguments according to the order of the function's parameters.
        new P4::OrderArguments(refMap, typeMap),
        new P4::ConvertEnums(refMap, typeMap, new EnumOn32Bits()),
        // Replace any slices in the left side of assignments and convert them to casts.
        new P4::RemoveLeftSlices(refMap, typeMap),
        new PassRepeated(
            {new P4::SimplifyControlFlow(refMap, typeMap),
             // Compress member access to struct expressions.
             new P4::ConstantFolding(refMap, typeMap),
             // Local copy propagation and dead-code elimination.
             new P4::LocalCopyPropagation(refMap, typeMap, nullptr,
                                          [](const Visitor::Context * /*context*/,
                                             const IR::Expression * /*expr*/) { return true; })}),
        // Remove loops from parsers by unrolling them as far as the stack indices allow.
        // TODO: Get rid of this pass.
        new P4::ParsersUnroll(true, refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        new P4::ConvertErrors(refMap, typeMap, new ErrorOn32Bits()),
        // Simplify header stack assignments with runtime indices into conditional statements.
        // TODO: Get rid of this pass.
        new P4::HSIndexSimplifier(refMap, typeMap),
        // Parse BMv2-specific annotations.
        new BMV2::ParseAnnotations(),
        // Convert Type_Varbits into a type that contains information about the assigned width.
        new ConvertVarbits(),
        // Cast all boolean table keys with a bit<1>.
        new P4::CastBooleanTableKeys(),
    });
    return midEnd;
}

}  // namespace P4Tools::Flay
