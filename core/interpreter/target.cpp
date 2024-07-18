#include "backends/p4tools/modules/flay/core/interpreter/target.h"

#include <string>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/compiler/convert_varbits.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/core/control_plane/bfruntime/protobuf.h"
#include "backends/p4tools/modules/flay/core/control_plane/p4runtime/protobuf.h"
#include "backends/p4tools/modules/flay/core/control_plane/protobuf_utils.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/toolname.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "backends/p4tools/modules/flay/core/control_plane/p4runtime/flaytests.pb.h"
#pragma GCC diagnostic pop

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
                                    ControlPlaneConstraints &constraints,
                                    ExecutionState &executionState) {
    return get().getStepperImpl(programInfo, constraints, executionState);
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

std::optional<ControlPlaneConstraints> FlayTarget::computeControlPlaneConstraintsImpl(
    const FlayCompilerResult &compilerResult, const FlayOptions &options) const {
    // Initialize some constraints that are active regardless of the control-plane
    // configuration. These constraints can be overridden by the respective control-plane
    // configuration.
    auto constraints = compilerResult.getDefaultControlPlaneConstraints();
    if (!options.hasControlPlaneConfig()) {
        return constraints;
    }
    auto confPath = options.controlPlaneConfig();
    printInfo("Parsing initial control plane configuration...\n");
    if (confPath.extension() == ".txtpb") {
        // By default we only support P4Runtime parsing.
        if (options.controlPlaneApi() == "P4RUNTIME") {
            auto deserializedConfig =
                Protobuf::deserializeObjectFromFile<p4runtime::flaytests::Config>(confPath);
            if (!deserializedConfig.has_value()) {
                return std::nullopt;
            }
            SymbolSet symbolSet;
            if (P4Runtime::updateControlPlaneConstraints(deserializedConfig.value(),
                                                         *compilerResult.getP4RuntimeApi().p4Info,
                                                         constraints, symbolSet) != EXIT_SUCCESS) {
                return std::nullopt;
            }
            return constraints;
        }
        if (options.controlPlaneApi() == "BFRUNTIME") {
            auto deserializedConfig =
                Protobuf::deserializeObjectFromFile<bfruntime::flaytests::Config>(confPath);
            if (!deserializedConfig.has_value()) {
                return std::nullopt;
            }
            SymbolSet symbolSet;
            if (BfRuntime::updateControlPlaneConstraints(deserializedConfig.value(),
                                                         *compilerResult.getP4RuntimeApi().p4Info,
                                                         constraints, symbolSet) != EXIT_SUCCESS) {
                return std::nullopt;
            }
            return constraints;
        }
    }

    ::error("Control plane file format %1% for control plane %2% not supported for this target.",
            confPath.extension().c_str(), options.controlPlaneApi().data());
    return std::nullopt;
}

MidEnd FlayTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    midEnd.setStopOnError(true);
    auto *refMap = midEnd.getRefMap();
    auto *typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Remove exit statements from the program.
        // TODO: We should not depend on this pass. It has bugs.
        new P4::RemoveExits(typeMap),
        // Replace types introduced by 'type' with 'typedef'.
        new P4::EliminateNewtype(refMap, typeMap),
        // Replace serializable enum constants with their values.
        new P4::EliminateSerEnums(refMap, typeMap),
        // Make sure that we have no TypeDef left in the program.
        new P4::EliminateTypedef(refMap, typeMap),
        // Sort call arguments according to the order of the function's parameters.
        new P4::OrderArguments(refMap, typeMap),
        // Replace any slices in the left side of assignments and convert them to casts.
        new P4::RemoveLeftSlices(refMap, typeMap),
        new PassRepeated({
            // Local copy propagation and dead-code elimination.
            new P4::LocalCopyPropagation(refMap, typeMap, nullptr,
                                         [](const Visitor::Context * /*context*/,
                                            const IR::Expression * /*expr*/) { return true; }),
            // Simplify control flow that has constants as conditions.
            new P4::SimplifyControlFlow(typeMap),
            // Compress member access to struct expressions.
            new P4::ConstantFolding(refMap, typeMap),
        }),
    });
    return midEnd;
}

PassManager FlayTarget::mkPrivateMidEnd(const CompilerOptions &options, P4::ReferenceMap *refMap,
                                        P4::TypeMap *typeMap) const {
    PassManager midEnd;
    midEnd.setName("FlayPrivateMidEnd");
    midEnd.setStopOnError(true);
    midEnd.addDebugHook(options.getDebugHook(), true);
    midEnd.addPasses({
        // Remove loops from parsers by unrolling them as far as the stack indices allow.
        // TODO: Get rid of this pass.
        new P4::ParsersUnroll(true, refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap, true),
        // Convert enums and errors to bit<32>.
        new P4::ConvertEnums(refMap, typeMap, new EnumOn32Bits()),
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

class P4ToolsFrontEndPolicy : public P4::FrontEndPolicy {
    [[nodiscard]] bool skipSideEffectOrdering() const override { return true; }
};

P4::FrontEnd FlayTarget::mkFrontEnd() const {
    if (FlayOptions::get().skipSideEffectOrdering()) {
        return P4::FrontEnd(new P4ToolsFrontEndPolicy());
    }
    return {};
}

ICompileContext *FlayTarget::makeContext() const {
    return new P4Tools::CompileContext<FlayOptions>();
}

}  // namespace P4Tools::Flay
