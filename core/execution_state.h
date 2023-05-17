#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_

#include <iostream>

#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
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

    /// The condition necessary to reach this particular execution state. Defaults to true.
    const IR::Expression *executionCondition;

    /// Keeps track of the parserStates which were visited to avoid infinite loops.
    std::set<int> visitedParserIds;

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

    /// Add a parser ID to the list of visited parser IDs.
    void addParserId(int parserId);

    /// @returns true if the parserID is already in the list of visited IDs.
    [[nodiscard]] bool hasVisitedParserId(int parserId) const;

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

    /// Push an execution condition into this particular state.
    void pushExecutionCondition(const IR::Expression *cond);

    /// @returns the execution condition associated with this state.
    [[nodiscard]] const IR::Expression *getExecutionCondition() const;

    /* =========================================================================================
     *  General utilities involving ExecutionState.
     * ========================================================================================= */
 public:
    /// Merge another execution state into this state.
    void merge(const ExecutionState &mergeState);

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

    /// Do not accidentally copy-assign the execution state.
    ExecutionState &operator=(const ExecutionState &) = delete;

    ExecutionState(ExecutionState &&) = default;

 private:
    /// Execution state needs to be explicitly copied using the @ref clone call..
    ExecutionState(const ExecutionState &) = default;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_ */
