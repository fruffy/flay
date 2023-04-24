#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_

#include <iostream>

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "frontends/p4/optimizeExpressions.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Represents state of execution after having reached a program point.
class ExecutionState {
 private:
    /// The namespace context in the IR for the current state. The innermost element is the P4
    /// program, representing the top-level namespace.
    const NamespaceContext *namespaces;

    /// The symbolic environment. Maps program variables to their symbolic values.
    SymbolicEnv env;

    /* =========================================================================================
     *  Accessors
     * ========================================================================================= */
 public:
    /// @returns the value associated with the given state variable.
    [[nodiscard]] const IR::Expression *get(const IR::StateVariable &var) const;

    /// Sets the symbolic value of the given state variable to the given value. Constant folding
    /// is done on the given value before updating the symbolic state.
    void set(const IR::StateVariable &var, const IR::Expression *value);

    /// Checks whether the given variable exists in the symbolic environment of this state.
    [[nodiscard]] bool exists(const IR::StateVariable &var) const;

    /// @returns the current symbolic environment.
    [[nodiscard]] const SymbolicEnv &getSymbolicEnv() const;

    /// Produce a formatted output of the current symbolic environment.
    void printSymbolicEnv(std::ostream &out = std::cout) const;

    /// @returns whether the property with @arg propertyName exists.
    [[nodiscard]] bool hasProperty(cstring propertyName) const;

 public:
    /// Looks up a declaration from a path. A BUG occurs if no declaration is found.
    [[nodiscard]] const IR::IDeclaration *findDecl(const IR::Path *path) const;

    /// Looks up a declaration from a path expression. A BUG occurs if no declaration is found.
    [[nodiscard]] const IR::IDeclaration *findDecl(const IR::PathExpression *pathExpr) const;

    /// Resolves a Type in the current environment.
    [[nodiscard]] const IR::Type *resolveType(const IR::Type *type) const;

    /// @returns the current namespace context.
    [[nodiscard]] const NamespaceContext *getNamespaceContext() const;

    /// Replaces the namespace context in the current state with the given context.
    void setNamespaceContext(const NamespaceContext *namespaces);

    /// Enters a namespace of declarations.
    void pushNamespace(const IR::INamespace *ns);

    /// Exists a namespace of declarations.
    void popNamespace();

    /* =========================================================================================
     *  General utilities involving ExecutionState.
     * ========================================================================================= */
 public:
    /// Merge another symbolic environment into this state under @param cond.
    void merge(const SymbolicEnv &mergeEnv, const IR::Expression *cond);

    /* =========================================================================================
     *  Constructors
     * ========================================================================================= */
    /// Creates an initial execution state for the given program.
    explicit ExecutionState(const IR::P4Program *program);

    /// Allocate a new execution state object with the same state as this object.
    /// Returns a reference, not a pointer.
    [[nodiscard]] ExecutionState &clone() const;

    /// Create a new execution state object from the input program.
    /// Returns a reference not a pointer.
    [[nodiscard]] static ExecutionState &create(const IR::P4Program *program);

    ExecutionState &operator=(const ExecutionState &) = delete;

 private:
    ExecutionState(const ExecutionState &) = default;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_ */
