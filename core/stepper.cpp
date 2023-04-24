#include "backends/p4tools/modules/flay/core/stepper.h"

#include <cstddef>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"

namespace P4Tools::Flay {

FlayStepper::FlayStepper(const ProgramInfo &programInfo, ExecutionState &executionState)
    : programInfo(programInfo), executionState(executionState) {}

const ProgramInfo &FlayStepper::getProgramInfo() const { return programInfo.get(); }

ExecutionState &FlayStepper::getExecutionState() const { return executionState.get(); }

bool FlayStepper::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Node %1% of type %2% not implemented in the core stepper.", node,
                      node->node_type_name());
}

void setStructLike(ExecutionState &state, const IR::Expression *target,
                   const IR::Expression *source) {
    const auto *typeTarget = target->type->checkedTo<IR::Type_StructLike>();
    const auto *typeSource = target->type->checkedTo<IR::Type_StructLike>();
    std::vector<const IR::Member *> flatTargetValids;
    std::vector<const IR::Member *> flatSourceValids;
    auto flatTargetFields = state.getFlatFields(target, typeTarget, &flatTargetValids);
    auto flatSourceFields = state.getFlatFields(source, typeSource, &flatSourceValids);
    BUG_CHECK(flatTargetFields.size() == flatSourceFields.size(),
              "The list of target fields and the list of source fields have different sizes.");
    for (size_t idx = 0; idx < flatTargetValids.size(); ++idx) {
        const auto *fieldTargetValid = flatTargetValids[idx];
        const auto *fieldSourceTarget = flatSourceValids[idx];
        state.set(fieldTargetValid, state.get(fieldSourceTarget));
    }
    // First, complete the assignments for the data structure.
    for (size_t idx = 0; idx < flatSourceFields.size(); ++idx) {
        const auto *flatTargetRef = flatTargetFields[idx];
        const auto *fieldSourceRef = flatSourceFields[idx];
        state.set(flatTargetRef, state.get(fieldSourceRef));
    }
}

void initializeStructLike(const ProgramInfo &programInfo, ExecutionState &state,
                          const IR::Expression *target, bool forceTaint) {
    const auto *typeTarget = target->type->checkedTo<IR::Type_StructLike>();
    std::vector<const IR::Member *> flatTargetValids;
    auto flatTargetFields = state.getFlatFields(target, typeTarget, &flatTargetValids);
    for (const auto *fieldTargetValid : flatTargetValids) {
        state.set(fieldTargetValid, IR::getBoolLiteral(false));
    }
    for (const auto *flatTargetRef : flatTargetFields) {
        state.set(flatTargetRef,
                  programInfo.createTargetUninitialized(flatTargetRef->type, forceTaint));
    }
}

void copyIn(ExecutionState &executionState, const ProgramInfo &programInfo,
            const IR::Parameter *internalParam, cstring externalParamName) {
    const auto *paramType = executionState.resolveType(internalParam->type);
    if (const auto *ts = paramType->to<IR::Type_StructLike>()) {
        const auto *externalParamRef =
            new IR::PathExpression(paramType, new IR::Path(externalParamName));
        const auto *internalParamRef =
            new IR::PathExpression(paramType, new IR::Path(internalParam->controlPlaneName()));
        if (internalParam->direction == IR::Direction::Out) {
            initializeStructLike(programInfo, executionState, internalParamRef, false);
        } else {
            setStructLike(executionState, internalParamRef, externalParamRef);
        }
    } else if (const auto *tb = paramType->to<IR::Type_Base>()) {
        const auto &externalParamRef =
            ToolsVariables::getStateVariable(paramType, externalParamName);
        const auto &internalParamRef =
            ToolsVariables::getStateVariable(paramType, internalParam->controlPlaneName());
        if (internalParam->direction == IR::Direction::Out) {
            executionState.set(internalParamRef, programInfo.createTargetUninitialized(tb, false));
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
    if (const auto *ts = paramType->to<IR::Type_StructLike>()) {
        const auto *externalParamRef =
            new IR::PathExpression(paramType, new IR::Path(externalParamName));
        const auto *internalParamRef =
            new IR::PathExpression(paramType, new IR::Path(internalParam->controlPlaneName()));
        if (internalParam->direction == IR::Direction::Out ||
            internalParam->direction == IR::Direction::InOut) {
            setStructLike(executionState, externalParamRef, internalParamRef);
        }
    } else if (const auto *tb = paramType->to<IR::Type_Base>()) {
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

IR::StateVariable convertReference(const IR::Expression *ref) {
    if (const auto *member = ref->to<IR::Member>()) {
        return member;
    }
    // Local variable.
    const auto *path = ref->checkedTo<IR::PathExpression>();
    return ToolsVariables::getStateVariable(path->type, path->path->name);
}

[[nodiscard]] std::vector<const IR::Expression *> getFlatStructFields(
    const IR::StructExpression *se) {
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

/* =============================================================================================
 *  Visitor functions
 * ============================================================================================= */

bool FlayStepper::preorder(const IR::P4Control *control) {
    auto &executionState = getExecutionState();
    // Enter the control's namespace.
    executionState.pushNamespace(control);
    auto blockName = control->getName().name;
    auto canonicalName = getProgramInfo().getCanonicalBlockName(blockName);
    const auto *controlParams = control->getApplyParameters();
    const auto *archSpec = FlayTarget::getArchSpec();
    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        copyIn(executionState, getProgramInfo(), internalParam, externalParamName);
    }
    control->body->apply_visitor_preorder(*this);

    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        copyOut(executionState, internalParam, externalParamName);
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::AssignmentStatement *assign) {
    const auto *left = assign->left;
    const auto *right = assign->right;
    auto &executionState = getExecutionState();
    const auto *assignType = left->type;
    assignType = executionState.resolveType(assignType);
    if (const auto *ts = assignType->to<IR::Type_StructLike>()) {
        if (const auto *structExpr = right->to<IR::StructExpression>()) {
            std::vector<const IR::Member *> flatTargetValids;
            auto flatTargetFields = executionState.getFlatFields(left, ts, &flatTargetValids);
            auto flatStructFields = getFlatStructFields(structExpr);
            BUG_CHECK(
                flatTargetFields.size() == flatStructFields.size(),
                "The list of target fields and the list of source fields have different sizes.");
            for (const auto *fieldTargetValid : flatTargetValids) {
                executionState.set(fieldTargetValid, IR::getBoolLiteral(true));
            }
            // First, complete the assignments for the data structure.
            for (size_t idx = 0; idx < flatTargetFields.size(); ++idx) {
                const auto *flatTargetRef = flatTargetFields[idx];
                const auto *flatStructField = flatStructFields[idx];
                executionState.set(flatTargetRef, flatStructField);
            }
        } else if (right->is<IR::PathExpression>() || right->is<IR::Member>()) {
            setStructLike(executionState, left, right);
        }
    } else if (const auto *tb = assignType->to<IR::Type_Base>()) {
        auto leftRef = convertReference(left);
        executionState.set(leftRef, right);
    } else {
        P4C_UNIMPLEMENTED("Unsupported assign type %1%", assignType->node_type_name());
    }

    return false;
}

bool FlayStepper::preorder(const IR::BlockStatement *block) {
    auto &executionState = getExecutionState();

    // Enter the block's namespace.
    executionState.pushNamespace(block);
    for (const auto *declOrStmt : block->components) {
        declOrStmt->apply_visitor_preorder(*this);
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::IfStatement *ifStmt) {
    auto &executionState = getExecutionState();

    // const auto *cond = ifStmt->condition;

    auto &trueState = executionState.clone();
    auto &trueStepper = FlayTarget::getStepper(programInfo, trueState);
    ifStmt->ifTrue->apply(trueStepper);

    if (ifStmt->ifFalse != nullptr) {
        ifStmt->ifFalse->apply_visitor_preorder(*this);
    }
    return false;
}

bool FlayStepper::preorder(const IR::MethodCallStatement *stmt) {
    stmt->methodCall->apply(ExpressionResolver(getProgramInfo(), getExecutionState()));
    return false;
}

void FlayStepper::initializeBlockParams(const IR::Type_Declaration *typeDecl,
                                        const std::vector<cstring> *blockParams,
                                        ExecutionState &nextState) const {
    // Collect parameters.
    const auto *iApply = typeDecl->to<IR::IApply>();
    BUG_CHECK(iApply != nullptr, "Constructed type %s of type %s not supported.", typeDecl,
              typeDecl->node_type_name());
    // Also push the namespace of the respective parameter.
    nextState.pushNamespace(typeDecl->to<IR::INamespace>());
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
        if (const auto *ts = paramType->to<IR::Type_StructLike>()) {
            const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(archRef));
            initializeStructLike(getProgramInfo(), nextState, paramRef, false);
        } else if (const auto *tb = paramType->to<IR::Type_Base>()) {
            const auto &paramRef = ToolsVariables::getStateVariable(paramType, archRef);
            // If the type is a flat Type_Base, postfix it with a "*".
            nextState.set(paramRef, getProgramInfo().createTargetUninitialized(paramType, false));
        } else {
            P4C_UNIMPLEMENTED("Unsupported initialization type %1%", paramType->node_type_name());
        }
    }
}

}  // namespace P4Tools::Flay
