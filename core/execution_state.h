#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_

#include <iostream>

#include "backends/p4tools/common/core/abstract_execution_state.h"
#include "backends/p4tools/common/lib/namespace_context.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Utility function to compare IR nodes in a set. We use their source info.
struct SourceIdCmp {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const;
};

using ReachabilityMap = std::map<const IR::Node *, const IR::Expression *, SourceIdCmp>;

/// Represents state of execution after having reached a program point.
class ExecutionState : public AbstractExecutionState {
 private:
    /// The condition necessary to reach this particular execution state. Defaults to true.
    const IR::Expression *executionCondition;

    /// Keeps track of the parserStates which were visited to avoid infinite loops.
    std::set<int> visitedParserIds;

    /// Keeps track of the reachability of individual nodes in the program.
    ReachabilityMap reachabilityMap;

    /* =========================================================================================
     *  Accessors
     * ========================================================================================= */
 public:
    /// @returns the value associated with the given state variable.
    [[nodiscard]] const IR::Expression *get(const IR::StateVariable &var) const override;

    /// Sets the symbolic value of the given state variable to the given value. Constant folding
    /// is done on the given value before updating the symbolic state.
    void set(const IR::StateVariable &var, const IR::Expression *value) override;

    /// Add a parser ID to the list of visited parser IDs.
    void addParserId(int parserId);

    /// @returns true if the parserID is already in the list of visited IDs.
    [[nodiscard]] bool hasVisitedParserId(int parserId) const;

    /* =========================================================================================
     *  State merging and execution conditions.
     * ========================================================================================= */
    /// Push an execution condition into this particular state.
    void pushExecutionCondition(const IR::Expression *cond);

    /// @returns the execution condition associated with this state.
    [[nodiscard]] const IR::Expression *getExecutionCondition() const;

    /// Merge another execution state into this state.
    void merge(const ExecutionState &mergeState);

    [[nodiscard]] const ReachabilityMap &getReachabilityMap() const;

    void addReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    const IR ::Expression *getReachabilityCondition(const IR::Node *node, bool checked) const;

    /* =========================================================================================
     *  Constructors
     * ========================================================================================= */
    /// Creates an initial execution state for the given program.
    explicit ExecutionState(const IR::P4Program *program);

    /// Allocate a new execution state object with the same state as this object.
    /// Returns a reference, not a pointer.
    [[nodiscard]] ExecutionState &clone() const override;

    /// Create a new execution state object from the input program.
    /// Returns a reference not a pointer.
    [[nodiscard]] static ExecutionState &create(const IR::P4Program *program);

    /// Do not accidentally copy-assign the execution state.
    ExecutionState &operator=(const ExecutionState &) = delete;

    ExecutionState &operator=(ExecutionState &&) = delete;

    ExecutionState(ExecutionState &&) = default;

    ~ExecutionState() override = default;

 private:
    /// Execution state needs to be explicitly copied using the @ref clone call..
    ExecutionState(const ExecutionState &) = default;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_ */
