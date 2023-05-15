#include "backends/p4tools/modules/flay/core/state_utils.h"

#include <cstddef>
#include <string>

#include "backends/p4tools/common/compiler/convert_hs_index.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay::StateUtils {

/* =========================================================================================
 *  General utilities involving ExecutionState.
 * ========================================================================================= */

std::vector<IR::StateVariable> getFlatFields(ExecutionState &state, const IR::Expression *parent,
                                             const IR::Type_StructLike *ts,
                                             std::vector<IR::StateVariable> *validVector) {
    std::vector<IR::StateVariable> flatFields;
    for (const auto *field : ts->fields) {
        const auto *fieldType = state.resolveType(field->type);
        if (const auto *ts = fieldType->to<IR::Type_StructLike>()) {
            auto subFields = getFlatFields(state, new IR::Member(fieldType, parent, field->name),
                                           ts, validVector);
            flatFields.insert(flatFields.end(), subFields.begin(), subFields.end());
        } else if (const auto *typeStack = fieldType->to<IR::Type_Stack>()) {
            const auto *stackElementsType = state.resolveType(typeStack->elementType);
            for (size_t arrayIndex = 0; arrayIndex < typeStack->getSize(); arrayIndex++) {
                const auto *newMember = HSIndexToMember::produceStackIndex(
                    stackElementsType, new IR::Member(typeStack, parent, field->name), arrayIndex);
                BUG_CHECK(stackElementsType->is<IR::Type_StructLike>(),
                          "Try to make the flat fields for non Type_StructLike element : %1%",
                          stackElementsType);
                auto subFields = getFlatFields(
                    state, newMember, stackElementsType->to<IR::Type_StructLike>(), validVector);
                flatFields.insert(flatFields.end(), subFields.begin(), subFields.end());
            }
        } else {
            flatFields.push_back(new IR::Member(fieldType, parent, field->name));
        }
    }
    // If we are dealing with a header we also include the validity bit in the list of
    // fields.
    if (validVector != nullptr && ts->is<IR::Type_Header>()) {
        validVector->push_back(ToolsVariables::getHeaderValidity(parent));
    }
    return flatFields;
}

void initializeStructLike(const ProgramInfo &programInfo, ExecutionState &state,
                          const IR::Expression *target, bool forceTaint) {
    const auto *typeTarget = target->type->checkedTo<IR::Type_StructLike>();
    std::vector<IR::StateVariable> flatTargetValids;
    auto flatTargetFields = getFlatFields(state, target, typeTarget, &flatTargetValids);
    for (const auto &fieldTargetValid : flatTargetValids) {
        state.set(fieldTargetValid, IR::getBoolLiteral(false));
    }
    for (const auto &flatTargetRef : flatTargetFields) {
        state.set(flatTargetRef,
                  programInfo.createTargetUninitialized(flatTargetRef->type, forceTaint));
    }
}

IR::StateVariable convertReference(const IR::Expression *ref) {
    if (const auto *member = ref->to<IR::Member>()) {
        return member;
    }
    // Local variable.
    const auto *path = ref->checkedTo<IR::PathExpression>();
    // return ToolsVariables::getStateVariable(path->type, path->path->name);
    return path;
}

const IR::P4Table *findTable(const ExecutionState &state, const IR::Member *member) {
    if (member->member != IR::IApply::applyMethodName) {
        return nullptr;
    }
    if (member->expr->is<IR::PathExpression>()) {
        const auto *declaration = state.findDecl(member->expr->to<IR::PathExpression>());
        return declaration->to<IR::P4Table>();
    }
    const auto *type = member->expr->type;
    if (const auto *tableType = type->to<IR::Type_Table>()) {
        return tableType->table;
    }
    return nullptr;
}

void setStructLike(ExecutionState &state, const IR::Expression *target,
                   const IR::Expression *source) {
    const auto *typeTarget = target->type->checkedTo<IR::Type_StructLike>();
    const auto *typeSource = target->type->checkedTo<IR::Type_StructLike>();
    std::vector<IR::StateVariable> flatTargetValids;
    std::vector<IR::StateVariable> flatSourceValids;
    auto flatTargetFields = getFlatFields(state, target, typeTarget, &flatTargetValids);
    auto flatSourceFields = getFlatFields(state, source, typeSource, &flatSourceValids);
    BUG_CHECK(flatTargetFields.size() == flatSourceFields.size(),
              "The list of target fields and the list of source fields have "
              "different sizes.");
    for (size_t idx = 0; idx < flatTargetValids.size(); ++idx) {
        const auto &fieldTargetValid = flatTargetValids[idx];
        const auto &fieldSourceTarget = flatSourceValids[idx];
        state.set(fieldTargetValid, state.get(fieldSourceTarget));
    }
    // First, complete the assignments for the data structure.
    for (size_t idx = 0; idx < flatSourceFields.size(); ++idx) {
        const auto &flatTargetRef = flatTargetFields[idx];
        const auto &fieldSourceRef = flatSourceFields[idx];
        state.set(flatTargetRef, state.get(fieldSourceRef));
    }
}

void declareVariable(const ProgramInfo &programInfo, ExecutionState &state,
                     const IR::Declaration_Variable &declVar) {
    if (declVar.initializer != nullptr) {
        P4C_UNIMPLEMENTED("Unsupported initializer %s for declaration variable.",
                          declVar.initializer);
    }
    const auto *declType = state.resolveType(declVar.type);

    if (const auto *structType = declType->to<IR::Type_StructLike>()) {
        const auto *parentExpr =
            new IR::PathExpression(structType, new IR::Path(declVar.name.name));
        initializeStructLike(programInfo, state, parentExpr, false);
    } else if (const auto *stackType = declType->to<IR::Type_Stack>()) {
        const auto *stackSizeExpr = stackType->size;
        auto stackSize = stackSizeExpr->checkedTo<IR::Constant>()->asInt();
        const auto *stackElemType = stackType->elementType;
        if (stackElemType->is<IR::Type_Name>()) {
            stackElemType = state.resolveType(stackElemType->to<IR::Type_Name>());
        }
        const auto *structType = stackElemType->checkedTo<IR::Type_StructLike>();
        for (auto idx = 0; idx < stackSize; idx++) {
            const auto *parentExpr = HSIndexToMember::produceStackIndex(
                structType, new IR::PathExpression(stackType, new IR::Path(declVar.name.name)),
                idx);
            initializeStructLike(programInfo, state, parentExpr, false);
        }
    } else if (declType->is<IR::Type_Base>()) {
        // If the variable does not have an initializer we need to create a new value for it. For
        // now we just use the name directly.
        const auto *left = new IR::PathExpression(declType, new IR::Path(declVar.name.name));
        state.set(left, programInfo.createTargetUninitialized(declType, false));
    } else {
        P4C_UNIMPLEMENTED("Unsupported declaration type %1% node: %2%", declType,
                          declType->node_type_name());
    }
}

void copyIn(ExecutionState &executionState, const ProgramInfo &programInfo,
            const IR::Parameter *internalParam, cstring externalParamName) {
    const auto *paramType = executionState.resolveType(internalParam->type);
    // We can not copy externs.
    if (paramType->is<IR::Type_Extern>()) {
        return;
    }
    if (paramType->is<IR::Type_StructLike>()) {
        const auto *externalParamRef =
            new IR::PathExpression(paramType, new IR::Path(externalParamName));
        const auto *internalParamRef =
            new IR::PathExpression(paramType, new IR::Path(internalParam->controlPlaneName()));
        if (internalParam->direction == IR::Direction::Out) {
            initializeStructLike(programInfo, executionState, internalParamRef, false);
        } else {
            setStructLike(executionState, internalParamRef, externalParamRef);
        }
    } else if (paramType->is<IR::Type_Base>()) {
        const auto &externalParamRef =
            ToolsVariables::getStateVariable(paramType, externalParamName);
        const auto &internalParamRef =
            ToolsVariables::getStateVariable(paramType, internalParam->controlPlaneName());
        if (internalParam->direction == IR::Direction::Out) {
            executionState.set(internalParamRef,
                               programInfo.createTargetUninitialized(paramType, false));
        } else {
            executionState.set(internalParamRef, executionState.get(externalParamRef));
        }
    } else {
        P4C_UNIMPLEMENTED("Unsupported copy-in type %1%", paramType->node_type_name());
    }
}

void copyOut(ExecutionState &executionState, const IR::Parameter *internalParam,
             cstring externalParamName) {
    const auto *paramType = executionState.resolveType(internalParam->type);
    // We can not copy externs.
    if (paramType->is<IR::Type_Extern>()) {
        return;
    }
    if (paramType->is<IR::Type_StructLike>()) {
        const auto *externalParamRef =
            new IR::PathExpression(paramType, new IR::Path(externalParamName));
        const auto *internalParamRef =
            new IR::PathExpression(paramType, new IR::Path(internalParam->controlPlaneName()));
        if (internalParam->direction == IR::Direction::Out ||
            internalParam->direction == IR::Direction::InOut) {
            setStructLike(executionState, externalParamRef, internalParamRef);
        }
    } else if (paramType->is<IR::Type_Base>()) {
        const auto &externalParamRef =
            ToolsVariables::getStateVariable(paramType, externalParamName);
        const auto &internalParamRef =
            ToolsVariables::getStateVariable(paramType, internalParam->controlPlaneName());
        if (internalParam->direction == IR::Direction::Out ||
            internalParam->direction == IR::Direction::InOut) {
            executionState.set(externalParamRef, executionState.get(internalParamRef));
        }
    } else {
        P4C_UNIMPLEMENTED("Unsupported copy-in type %1%", paramType->node_type_name());
    }
}

std::vector<const IR::Expression *> getFlatStructFields(const IR::StructExpression *se) {
    std::vector<const IR::Expression *> flatStructFields;
    for (const auto *field : se->components) {
        if (const auto *structExpr = field->expression->to<IR::StructExpression>()) {
            auto subFields = getFlatStructFields(structExpr);
            flatStructFields.insert(flatStructFields.end(), subFields.begin(), subFields.end());
        } else {
            flatStructFields.push_back(field->expression);
        }
    }
    return flatStructFields;
}

void initializeBlockParams(const ProgramInfo &programInfo, ExecutionState &state,
                           const IR::Type_Declaration *typeDecl,
                           const std::vector<cstring> *blockParams) {
    // Collect parameters.
    const auto *iApply = typeDecl->to<IR::IApply>();
    BUG_CHECK(iApply != nullptr, "Constructed type %s of type %s not supported.", typeDecl,
              typeDecl->node_type_name());
    // Also push the namespace of the respective parameter.
    state.pushNamespace(typeDecl->to<IR::INamespace>());
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
        paramType = state.resolveType(paramType);
        if (paramType->is<IR::Type_StructLike>()) {
            const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(archRef));
            initializeStructLike(programInfo, state, paramRef, false);
        } else if (paramType->is<IR::Type_Base>()) {
            const auto &paramRef = ToolsVariables::getStateVariable(paramType, archRef);
            state.set(paramRef, programInfo.createTargetUninitialized(paramType, false));
        } else {
            P4C_UNIMPLEMENTED("Unsupported initialization type %1%", paramType->node_type_name());
        }
    }
}

const IR::P4Action *getP4Action(ExecutionState &state,
                                const IR::MethodCallExpression *tableAction) {
    const auto *actionPath = tableAction->method->checkedTo<IR::PathExpression>();
    const auto *declaration = state.findDecl(actionPath);
    return declaration->checkedTo<IR::P4Action>();
}

}  // namespace P4Tools::Flay::StateUtils
