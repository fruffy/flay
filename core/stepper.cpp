#include "backends/p4tools/modules/flay/core/stepper.h"

#include <cstddef>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/target.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
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

const IR::Expression *defaultValue(ExecutionState &nextState, const Util::SourceInfo &srcInfo,
                                   const IR::Type *type) {
    if (const auto *tb = type->to<IR::Type_Bits>()) {
        return new IR::Constant(srcInfo, tb, 0);
    }
    if (type->is<IR::Type_InfInt>()) {
        return new IR::Constant(srcInfo, 0);
    }
    if (type->is<IR::Type_Boolean>()) {
        return new IR::BoolLiteral(srcInfo, false);
    }
    if (const auto *te = type->to<IR::Type_Enum>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name),
                              te->members.at(0)->getName());
    }
    if (const auto *te = type->to<IR::Type_SerEnum>()) {
        return new IR::Cast(srcInfo, type->getP4Type(), new IR::Constant(srcInfo, te->type, 0));
    }
    if (const auto *te = type->to<IR::Type_Error>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name), "NoError");
    }
    if (type->is<IR::Type_String>()) {
        return new IR::StringLiteral(srcInfo, cstring(""));
    }
    if (type->is<IR::Type_Varbits>()) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1% default values for varbit types", srcInfo);
        return nullptr;
    }
    if (const auto *ht = type->to<IR::Type_Header>()) {
        return new IR::InvalidHeader(ht->getP4Type());
    }
    if (const auto *hu = type->to<IR::Type_HeaderUnion>()) {
        return new IR::InvalidHeaderUnion(hu->getP4Type());
    }
    if (const auto *st = type->to<IR::Type_StructLike>()) {
        auto *vec = new IR::IndexedVector<IR::NamedExpression>();
        for (const auto *field : st->fields) {
            const auto *fieldType = nextState.resolveType(field->type);
            const auto *value = defaultValue(nextState, srcInfo, fieldType);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(new IR::NamedExpression(field->name, value));
        }
        const auto *resultType = st->getP4Type();
        return new IR::StructExpression(srcInfo, resultType, resultType, *vec);
    }
    if (const auto *tf = type->to<IR::Type_Fragment>()) {
        return defaultValue(nextState, srcInfo, tf->type);
    }
    if (const auto *tt = type->to<IR::Type_BaseList>()) {
        auto *vec = new IR::Vector<IR::Expression>();
        for (const auto *field : tt->components) {
            const auto *fieldType = nextState.resolveType(field);
            const auto *value = defaultValue(nextState, srcInfo, fieldType);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(value);
        }
        return new IR::ListExpression(srcInfo, *vec);
    }
    if (const auto *ts = type->to<IR::Type_Stack>()) {
        auto *vec = new IR::Vector<IR::Expression>();
        const auto *elementType = ts->elementType;
        for (size_t i = 0; i < ts->getSize(); i++) {
            const IR::Expression *invalid = nullptr;
            if (elementType->is<IR::Type_Header>()) {
                invalid = new IR::InvalidHeader(elementType->getP4Type());
            } else {
                BUG_CHECK(elementType->is<IR::Type_HeaderUnion>(),
                          "%1%: expected a header or header union stack", elementType);
                invalid = new IR::InvalidHeaderUnion(srcInfo, elementType->getP4Type());
            }
            vec->push_back(invalid);
        }
        const auto *resultType = ts->getP4Type();
        return new IR::HeaderStackExpression(srcInfo, resultType, *vec, resultType);
    }
    ::error(ErrorType::ERR_INVALID, "%1%: No default value for type %2%", srcInfo, type);
    return nullptr;
}

bool FlayStepper::preorder(const IR::Node *node) {
    P4C_UNIMPLEMENTED("Node %1% of type %2% not implemented in the core stepper.", node,
                      node->node_type_name());
}

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
        const auto *paramType = internalParam->type;
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        paramType = executionState.resolveType(paramType);
        const auto *externalParamPath =
            new IR::PathExpression(paramType, new IR::Path(externalParamName));
        const auto &externalParamRef = new IR::Member(paramType, externalParamPath, "*");
        const auto *internalParamPath =
            new IR::PathExpression(internalParam->getSourceInfo(), paramType,
                                   new IR::Path(internalParam->controlPlaneName()));
        const auto &internalParamRef = new IR::Member(paramType, internalParamPath, "*");
        if (internalParam->direction == IR::Direction::Out) {
            executionState.set(internalParamRef,
                               defaultValue(executionState, Util::SourceInfo(), paramType));
        } else {
            executionState.set(internalParamRef, executionState.get(externalParamRef));
        }
    }
    control->body->apply_visitor_preorder(*this);

    for (size_t paramIdx = 0; paramIdx < controlParams->size(); ++paramIdx) {
        const auto *internalParam = controlParams->getParameter(paramIdx);
        const auto *paramType = internalParam->type;
        auto externalParamName = archSpec->getParamName(canonicalName, paramIdx);
        paramType = executionState.resolveType(paramType);
        const auto *externalParamPath =
            new IR::PathExpression(paramType, new IR::Path(externalParamName));
        const auto &externalParamRef = new IR::Member(paramType, externalParamPath, "*");
        const auto *internalParamPath =
            new IR::PathExpression(internalParam->getSourceInfo(), paramType,
                                   new IR::Path(internalParam->controlPlaneName()));
        const auto &internalParamRef = new IR::Member(paramType, internalParamPath, "*");
        if (internalParam->direction == IR::Direction::Out ||
            internalParam->direction == IR::Direction::InOut) {
            executionState.set(externalParamRef, executionState.get(internalParamRef));
        }
    }
    executionState.popNamespace();
    return false;
}

bool FlayStepper::preorder(const IR::AssignmentStatement * /*assign*/) { return false; }

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

    auto trueState = executionState.clone();
    auto &trueStepper = FlayTarget::getStepper(programInfo, executionState);
    ifStmt->ifTrue->apply(trueStepper);

    if (ifStmt->ifFalse != nullptr) {
        ifStmt->ifFalse->apply_visitor_preorder(*this);
    }
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
        const auto *paramPath = new IR::PathExpression(paramType, new IR::Path(archRef));
        const auto &paramRef = new IR::Member(paramType, paramPath, "*");
        const auto *val = defaultValue(nextState, Util::SourceInfo(), paramType);
        nextState.set(paramRef, val);
    }
}

}  // namespace P4Tools::Flay
