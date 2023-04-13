#include "backends/p4tools/modules/flay/core/stepper.h"

#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

FlayStepper::FlayStepper(const ProgramInfo &programInfo, ExecutionState &executionState)
    : programInfo(programInfo), executionState(executionState) {}

const ProgramInfo &FlayStepper::getProgramInfo() const { return programInfo.get(); }

ExecutionState &FlayStepper::getExecutionState() const { return executionState.get(); }

void FlayStepper::declareStructLike(ExecutionState &nextState, const IR::Expression *parentExpr,
                                    const IR::Type_StructLike *structType, bool forceTaint) const {}

void FlayStepper::initializeBlockParams(const IR::Type_Declaration *typeDecl,
                                        const std::vector<cstring> *blockParams,
                                        ExecutionState &nextState, bool forceTaint) const {
    // Collect parameters.
    const auto *iApply = typeDecl->to<IR::IApply>();
    BUG_CHECK(iApply != nullptr, "Constructed type %s of type %s not supported.", typeDecl,
              typeDecl->node_type_name());
    // Also push the namespace of the respective parameter.
    // nextState.pushNamespace(typeDecl->to<IR::INamespace>());
    // Collect parameters.
    const auto *params = iApply->getApplyParameters();
    for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
        const auto *param = params->getParameter(paramIdx);
        const auto *paramType = param->type;
        // Retrieve the identifier of the global architecture map using the parameter index.
        auto archRef = blockParams->at(paramIdx);
        // Irrelevant parameter. Ignore.
        if (archRef == nullptr) {
            continue;
        }
        // We need to resolve type names.
        paramType = nextState.resolveType(paramType);
    }
}

}  // namespace P4Tools::Flay
