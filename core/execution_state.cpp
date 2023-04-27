#include "backends/p4tools/modules/flay/core/execution_state.h"

#include <ostream>
#include <utility>

#include <boost/container/vector.hpp>

#include "backends/p4tools/common/compiler/collapse_mux.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4Tools::Flay {

ExecutionState::ExecutionState(const IR::P4Program *program)
    : namespaces(NamespaceContext::Empty->push(program)) {}

/* =============================================================================================
 *  Accessors
 * ============================================================================================= */

const IR::Expression *ExecutionState::get(const IR::StateVariable &var) const {
    // TODO: This is a convoluted (and expensive?) check because struct members are not directly
    // associated with a header. We should be using runtime objects instead of flat assignments.
    const auto *expr = env.get(var);
    if (const auto *member = var->to<IR::Member>()) {
        if (member->expr->type->is<IR::Type_Header>() && member->member != ToolsVariables::VALID) {
            // If we are setting the member of a header, we need to check whether the
            // header is valid.
            // If the header is invalid, the get returns a tainted expression.
            // The member could have any value.
            auto validity = ToolsVariables::getHeaderValidity(member->expr);
            const auto *validVar = env.get(validity);
            if (const auto *validBool = validVar->to<IR::BoolLiteral>()) {
                if (validBool->value) {
                    return expr;
                }
                return IR::getDefaultValue(expr->type);
            }
            return new IR::Mux(expr->type, validVar, expr, IR::getDefaultValue(expr->type));
        }
    }
    return expr;
}

bool ExecutionState::exists(const IR::StateVariable &var) const { return env.exists(var); }

void ExecutionState::set(const IR::StateVariable &var, const IR::Expression *value) {
    // Small optimization. Do not nest Mux that are the same.
    if (const auto *mux = value->to<IR::Mux>()) {
        value = value->apply(CollapseMux());
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

void ExecutionState::merge(const SymbolicEnv &mergeEnv, const IR::Expression *cond) {
    cond = P4::optimizeExpression(cond);
    if (const auto *boolExpr = cond->to<IR::BoolLiteral>()) {
        // If the condition is false, do nothing. If it is true, set all the values.
        if (boolExpr->value) {
            for (const auto &envTuple : mergeEnv.getInternalMap()) {
                auto ref = envTuple.first;
                if (exists(ref)) {
                    set(ref, envTuple.second);
                }
            }
        }
        return;
    }
    for (const auto &envTuple : mergeEnv.getInternalMap()) {
        auto ref = envTuple.first;
        const auto *mergeExpr = envTuple.second;
        if (exists(ref)) {
            const auto *currentExpr = get(ref);
            auto *mergedExpr = new IR::Mux(currentExpr->type, cond, mergeExpr, currentExpr);
            set(envTuple.first, mergedExpr);
        }
    }
}

/* =========================================================================================
 *  Constructors
 * ========================================================================================= */

ExecutionState &ExecutionState::create(const IR::P4Program *program) {
    return *new ExecutionState(program);
}

ExecutionState &ExecutionState::clone() const { return *new ExecutionState(*this); }

}  // namespace P4Tools::Flay
