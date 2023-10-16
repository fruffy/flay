#include "backends/p4tools/modules/flay/core/target.h"

#include <string>
#include <utility>

#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay {

FlayTarget::FlayTarget(std::string deviceName, std::string archName)
    : Target("flay", std::move(deviceName), std::move(archName)) {}

const ProgramInfo *FlayTarget::initProgramImpl(const IR::P4Program *program) const {
    // Check that the program has at least one main declaration.
    const auto mainCount = program->getDeclsByName(IR::P4Program::main)->count();
    BUG_CHECK(mainCount > 0, "Program doesn't have a main declaration.");

    // Resolve the program's main declaration instance and delegate to the version of
    // initProgramImpl that takes the main declaration.
    const auto *mainIDecl = program->getDeclsByName(IR::P4Program::main)->single();
    BUG_CHECK(mainIDecl, "Program's main declaration not found: %1%", program->main);

    const auto *mainNode = mainIDecl->getNode();
    const auto *mainDecl = mainIDecl->to<IR::Declaration_Instance>();
    BUG_CHECK(mainDecl, "%1%: Program's main declaration is a %2%, not a Declaration_Instance",
              mainNode, mainNode->node_type_name());

    return initProgramImpl(program, mainDecl);
}

const FlayTarget &FlayTarget::get() { return Target::get<FlayTarget>("flay"); }

const ArchSpec *FlayTarget::getArchSpec() { return get().getArchSpecImpl(); }

FlayStepper &FlayTarget::getStepper(const ProgramInfo &programInfo,
                                    ExecutionState &executionState) {
    return get().getStepperImpl(programInfo, executionState);
}

std::optional<ControlPlaneConstraints> FlayTarget::computeControlPlaneConstraints(
    const IR::P4Program &program, const FlayOptions &options) {
    return get().computeControlPlaneConstraintsImpl(program, options);
}

void FlayTarget::argumentsToTypeDeclarations(
    const IR::IGeneralNamespace *ns, const IR::Vector<IR::Argument> *inputArgs,
    std::vector<const IR::Type_Declaration *> &resultDecls) {
    for (const auto *arg : *inputArgs) {
        const auto *expr = arg->expression;

        const IR::Type_Declaration *declType = nullptr;

        if (const auto *ctorCall = expr->to<IR::ConstructorCallExpression>()) {
            const auto *constructedTypeName = ctorCall->constructedType->checkedTo<IR::Type_Name>();

            // Find the corresponding type declaration in the namespace.
            declType = ProgramInfo::findProgramDecl(ns, constructedTypeName->path)
                           ->checkedTo<IR::Type_Declaration>();
        } else if (const auto *pathExpr = expr->to<IR::PathExpression>()) {
            // Look up the path expression in the declaration map, and expect to find a
            // declaration instance.
            const auto *declInstance = ProgramInfo::findProgramDecl(ns, pathExpr->path)
                                           ->checkedTo<IR::Declaration_Instance>();
            declType = declInstance->type->checkedTo<IR::Type_Declaration>();
        } else {
            BUG("Unexpected main-declaration argument node type: %1%", expr->node_type_name());
        }

        // The constructor's parameter list should be empty, since the compiler should have
        // substituted the constructor arguments for us.
        if (const auto *iApply = declType->to<IR::IContainer>()) {
            const IR::ParameterList *ctorParams = iApply->getConstructorParameters();
            BUG_CHECK(ctorParams->empty(), "Compiler did not eliminate constructor parameters: %1%",
                      ctorParams);
        } else {
            BUG("Does not instantiate an IContainer: %1%", expr);
        }

        resultDecls.emplace_back(declType);
    }
}

const ProgramInfo *FlayTarget::initProgram(const IR::P4Program *program) {
    return get().initProgramImpl(program);
}

}  // namespace P4Tools::Flay
