#include "backends/p4tools/modules/flay/passes/substitute_expressions.h"

#include <optional>

#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/modules/flay/control_plane/return_macros.h"
#include "ir/node.h"
#include "ir/vector.h"

namespace P4Tools::Flay {

SubstituteExpressions::SubstituteExpressions(const P4::ReferenceMap &refMap,
                                             const AbstractSubstitutionMap &substitutionMap)
    : _substitutionMap(substitutionMap), _refMap(refMap) {}

const IR::Node *SubstituteExpressions::preorder(IR::Declaration_Variable *declaration) {
    if (declaration->initializer != nullptr) {
        declaration->initializer =
            declaration->initializer->apply(SubstituteExpressions(_refMap, _substitutionMap));
    }
    return declaration;
}

const IR::Node *SubstituteExpressions::preorder(IR::AssignmentStatement *statement) {
    // Only analyze the right hand side of the assignment.
    prune();
    statement->right = statement->right->apply(SubstituteExpressions(_refMap, _substitutionMap));
    return statement;
}

const IR::Node *SubstituteExpressions::preorder(IR::MethodCallExpression *call) {
    // Do not bother checking calls in action lists.
    if (findContext<IR::ActionListElement>() != nullptr) {
        return call;
    }
    prune();
    const auto *callType = call->method->type;
    const IR::ParameterList *paramList = nullptr;
    if (const auto *methodType = callType->to<IR::Type_Method>()) {
        paramList = methodType->parameters;
    } else if (const auto *actionType = callType->to<IR::Type_Action>()) {
        paramList = actionType->parameters;
    } else {
        ::error("Unexpected type %1% for call %2%.", callType, call);
        return call;
    }
    if (call->arguments->size() > paramList->size()) {
        ::error("%1%: Expected lesser or equals than %2% arguments, got %3%.", call,
                paramList->size(), call->arguments->size());
        return call;
    }
    bool hasChanged = false;
    auto *argumentList = new IR::Vector<IR::Argument>();
    for (size_t idx = 0; idx < call->arguments->size(); idx++) {
        const auto *parameter = paramList->parameters.at(idx);
        if (parameter->direction == IR::Direction::InOut ||
            parameter->direction == IR::Direction::Out) {
            argumentList->push_back(call->arguments->at(idx));
            continue;
        }
        hasChanged = true;
        auto subst = SubstituteExpressions(_refMap, _substitutionMap);
        const auto *ret = call->arguments->at(idx)->apply(subst);
        ASSIGN_OR_RETURN_WITH_MESSAGE(auto &newArg, ret->to<IR::Argument>(), call,
                                      ::error("Resolved argument %1% is not an argument.", ret));
        argumentList->push_back(&newArg);
    }
    if (hasChanged) {
        call->arguments = argumentList;
    }
    return call;
}

const IR::Node *SubstituteExpressions::preorder(IR::PathExpression *pathExpression) {
    if (!pathExpression->getSourceInfo().isValid() || !pathExpression->type->is<IR::Type_Bits>()) {
        return pathExpression;
    }
    auto optConstant = _substitutionMap.get().isExpressionConstant(pathExpression);
    if (optConstant.has_value()) {
        printInfo("---SUBSTITUTION--- Replacing %1% with %2%.", pathExpression,
                  optConstant.value());
        _eliminatedNodes.emplace_back(pathExpression, optConstant.value());
        return optConstant.value();
    }
    return pathExpression;
}

const IR::Node *SubstituteExpressions::preorder(IR::Member *member) {
    if (!member->getSourceInfo().isValid() || !member->type->is<IR::Type_Bits>()) {
        return member;
    }

    auto optConstant = _substitutionMap.get().isExpressionConstant(member);
    if (optConstant.has_value()) {
        printInfo("---SUBSTITUTION--- Replacing %1% with %2%.", member, optConstant.value());
        _eliminatedNodes.emplace_back(member, optConstant.value());
        return optConstant.value();
    }
    return member;
}

std::vector<EliminatedReplacedPair> SubstituteExpressions::eliminatedNodes() const {
    return _eliminatedNodes;
}

}  // namespace P4Tools::Flay
