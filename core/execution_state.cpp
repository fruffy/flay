#include "backends/p4tools/modules/flay/core/execution_state.h"

#include <ostream>
#include <stack>
#include <utility>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools::Flay {

ExecutionState::ExecutionState(const IR::P4Program *program)
    : namespaces(NamespaceContext::Empty->push(program)) {}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    if (var->expr->type->is<IR::Type_StructLike>() && var->type->is<IR::Type_Base>()) {
        auto *expr = env.get(var->expr->checkedTo<IR::Member>());
        if (expr->is<IR::InvalidHeader>()) {
            return ToolsVariables::getTaintExpression(var->type);
        }

        while (auto *structExpr = expr->to<IR::StructExpression>()) {
        }
    }
    return env.get(var);
}

bool ExecutionState::exists(const IR::StateVariable &var) const { return env.exists(var); }

void ExecutionState::set(const IR::StateVariable &var, IR::Expression *value) {
    static cstring VAR_POSTFIX = "p4tools*var";
    if (var->expr->is<IR::Member>()) {
        std::stack<cstring> refs;
        const IR::Expression *expr = var->expr;
        while (const auto *member = expr->to<IR::Member>()) {
            refs.emplace(member->member);
            expr = member->expr;
        }
        const auto *path = expr->checkedTo<IR::PathExpression>();
        auto *refExpr = env.get(new IR::Member(path, VAR_POSTFIX));
        if (refExpr->is<IR::InvalidHeader>()) {
            return;
        }
        while (auto ref = refs.top()) {
            refs.pop();
            auto *structExpr = refExpr->to<IR::StructExpression>();
            CHECK_NULL(structExpr);
            if (refs.empty()) {
                auto &components = structExpr->components;
                for (auto it = components.begin(); it != components.end(); ++it) {
                    const auto *decl = (*it)->template to<IR::IDeclaration>();
                    if (decl != nullptr && decl->getName() == var->member) {
                        components.replace(it, new IR::NamedExpression(ref, var));
                    }
                }
            } else {
                refExpr = structExpr->components.getDeclaration(ref);
            }
        }

        return;
    }
    env.set(var, value);
}

const SymbolicEnv &ExecutionState::getSymbolicEnv() const { return env; }

void ExecutionState::printSymbolicEnv(std::ostream &out) const {
    // TODO(fruffy): How do we do logging here?
    out << "##### Symbolic Environment Begin #####" << std::endl;
    for (const auto &envVar : env.getInternalMap()) {
        const auto var = envVar.first;
        const auto *val = envVar.second;
        out << "Variable: " << var->toString() << " Value: " << val << std::endl;
    }
    out << "##### Symbolic Environment End #####" << std::endl;
}

IR::StateVariable ExecutionState::convertReference(const IR::Expression *ref) {
    static cstring VAR_POSTFIX = "p4tools*var";
    if (const auto *member = ref->to<IR::Member>()) {
        return {new IR::Member(ref, VAR_POSTFIX)};
    }
    // Local variable.
    const auto *path = ref->checkedTo<IR::PathExpression>();
    return {new IR::Member(path, VAR_POSTFIX)};
}

IR::StateVariable ExecutionState::createStateVariable(const IR::Type *type, cstring name) {
    static cstring VAR_POSTFIX = "p4tools*var";
    return {new IR::Member(new IR::PathExpression(type, new IR::Path(name)), VAR_POSTFIX)};
}

/* =============================================================================================
 *  Namespaces and declarations
 * ============================================================================================= */

const IR::IDeclaration *ExecutionState::findDecl(const IR::Path *path) const {
    return namespaces->findDecl(path);
}

const IR::IDeclaration *ExecutionState::findDecl(const IR::PathExpression *pathExpr) const {
    return findDecl(pathExpr->path);
}

const IR::Type *ExecutionState::resolveType(const IR::Type *type) const {
    const auto *typeName = type->to<IR::Type_Name>();
    // Nothing to resolve here. Just return.
    if (typeName == nullptr) {
        return type;
    }
    const auto *path = typeName->path;
    const auto *decl = findDecl(path)->to<IR::Type_Declaration>();
    BUG_CHECK(decl, "Not a type: %1%", path);
    return decl;
}

const NamespaceContext *ExecutionState::getNamespaceContext() const { return namespaces; }

void ExecutionState::setNamespaceContext(const NamespaceContext *namespaces) {
    this->namespaces = namespaces;
}

void ExecutionState::pushNamespace(const IR::INamespace *ns) { namespaces = namespaces->push(ns); }

void ExecutionState::popNamespace() { namespaces = namespaces->pop(); }

/* =========================================================================================
 *  Constructors
 * ========================================================================================= */

ExecutionState &ExecutionState::create(const IR::P4Program *program) {
    return *new ExecutionState(program);
}

ExecutionState &ExecutionState::clone() const { return *new ExecutionState(*this); }

}  // namespace P4Tools::Flay
