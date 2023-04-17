#include "backends/p4tools/modules/flay/core/execution_state.h"

#include <ostream>
#include <utility>

#include <boost/container/vector.hpp>

#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools::Flay {

ExecutionState::ExecutionState(const IR::P4Program *program)
    : namespaces(NamespaceContext::Empty->push(program)) {}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

const IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    const auto *expr = env.get(var);
    return expr;
}

bool ExecutionState::exists(const IR::StateVariable &var) const { return env.exists(var); }

void ExecutionState::set(const IR::StateVariable &var, const IR::Expression *value) {
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
