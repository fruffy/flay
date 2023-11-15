#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXECUTION_STATE_H_

#include <map>
#include <set>

#include "backends/p4tools/common/core/abstract_execution_state.h"
#include "ir/ir.h"
#include "ir/node.h"

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

    /// A static label for placeholder variables used in Flay.
    static const IR::PathExpression PLACEHOLDER_LABEL;
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

    /// @returns a symbolic expression using the id and label provided.
    /// Also handles complex expressions such as structs or headers.
    const IR::Expression *createSymbolicExpression(const IR::Type *inputType, cstring label) const;

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

    /// Map the conditions to be reachable to a particular program node.
    void addReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    /// @returns the set of conditions required to reach a particular node in the program.
    /// Throws an error if the node is not present in the program.
    const IR ::Expression *getReachabilityCondition(const IR::Node *node, bool checked) const;

    /// Convenience function to set the value of a placeholder variables in the symbolic
    /// environment. An example where placeholder variables are necessary is recirculation. We can
    /// not immediately know the conditions to cover a particular program node and need to assign a
    /// placeholder.
    void setPlaceholderValue(cstring label, const IR::Expression *value);

    /// @returns the assigned value for a particular placeholder in this execution state.
    /// Returns std::nullopt if no value was found.
    std::optional<const IR::Expression *> getPlaceholderValue(
        const IR::Placeholder &placeholder) const;

    /// Substitute IR::Placeholder expression in all reachability conditions presents in this
    /// execution state.
    const ExecutionState &substitutePlaceholders() const;

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
