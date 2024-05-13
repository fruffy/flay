#include "backends/p4tools/modules/flay/core/target.h"

#include <string>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/convert_varbits.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/toolname.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"
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

namespace P4Tools::Flay {

FlayTarget::FlayTarget(const std::string &deviceName, const std::string &archName)
    : CompilerTarget(TOOL_NAME, deviceName, archName) {}

const ProgramInfo *FlayTarget::produceProgramInfoImpl(const CompilerResult &compilerResult) const {
    const auto &program = compilerResult.getProgram();
    // Check that the program has at least one main declaration.
    const auto mainCount = program.getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");

    // Resolve the program's main declaration instance and delegate to the version of
    // produceProgramInfoImpl that takes the main declaration.
    const auto *mainIDecl = program.getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program.main);

    const auto *mainNode = mainIDecl->getNode();
    const auto *mainDecl = mainIDecl->to<IR::Declaration_Instance>();
    BUG_CHECK(mainDecl, "%1%: Program's main declaration is a %2%, not a Declaration_Instance",
              mainNode, mainNode->node_type_name());

    return produceProgramInfoImpl(compilerResult, mainDecl);
}

const FlayTarget &FlayTarget::get() { return Target::get<FlayTarget>(TOOL_NAME); }

const ArchSpec *FlayTarget::getArchSpec() { return get().getArchSpecImpl(); }

FlayStepper &FlayTarget::getStepper(const ProgramInfo &programInfo,
                                    ExecutionState &executionState) {
    return get().getStepperImpl(programInfo, executionState);
}

std::optional<ControlPlaneConstraints> FlayTarget::computeControlPlaneConstraints(
    const FlayCompilerResult &compilerResult, const FlayOptions &options) {
    return get().computeControlPlaneConstraintsImpl(compilerResult, options);
}

const ProgramInfo *FlayTarget::produceProgramInfo(const CompilerResult &compilerResult) {
    return get().produceProgramInfoImpl(compilerResult);
}

/// Implements the default enum-conversion policy, which converts all enums to bit<32>.
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum * /*type*/) const override { return true; }

    [[nodiscard]] unsigned enumSize(unsigned /*enumCount*/) const override { return 32; }
};

class ErrorOn32Bits : public P4::ChooseErrorRepresentation {
    bool convert(const IR::Type_Error * /*type*/) const override { return true; }

    [[nodiscard]] unsigned errorSize(unsigned /*errorCount*/) const override { return 32; }
};

MidEnd FlayTarget::mkMidEnd(const CompilerOptions &options) const {
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
