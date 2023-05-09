#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"

namespace P4Tools::Flay {

/// Forward declaration of the calling expression resolver.
class ExpressionResolver;

/// Executes a table and synthesizes control plane action parameters.
class TableExecutor {
 private:
    static const IR::Type_Bits ACTION_BIT_TYPE;

    /// The table associated with this executor.
    std::reference_wrapper<const IR::P4Table> table;

    /// The resolver that instantiated this table execution..
    std::reference_wrapper<ExpressionResolver> resolver;

    /// Resolves the input key and ensures that all members of the key are pure symbolic.
    /// @returns the symbolic key.
    const IR::Key *resolveKey(const IR::Key *key) const;

    const IR::Expression *computeKey(const IR::Key *key) const;

    /// Handle the default action.
    void processDefaultAction() const;

    /// Process all the possible actions in the table for which we could insert an entry.
    void processTableActionOptions(const IR::SymbolicVariable *tableActionID,
                                   const IR::Key *key) const;

 protected:
    /// @returns the table associated with this executor.
    [[nodiscard]] const IR::P4Table &getP4Table() const;

    /// @returns the current execution state.
    [[nodiscard]] ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    [[nodiscard]] const ProgramInfo &getProgramInfo() const;

    /// Compute a table key match. Can be overridden by targets to add more match types.
    /// @returns the full match expression for the key given the table match types.
    virtual const IR::Expression *computeTargetMatchType(const IR::KeyElement *keyField) const;

 public:
    /// Execute the table and @return the state after executing it (hit, action_run).
    const IR::Expression *processTable();

    /// Helper function to call an action with arguments.
    static void callAction(const ProgramInfo &programInfo, ExecutionState &state,
                           const IR::P4Action *actionType,
                           const IR::Vector<IR::Argument> &arguments);

    explicit TableExecutor(const IR::P4Table &table, ExpressionResolver &callingResolver);
    TableExecutor(const TableExecutor &) = default;
    TableExecutor(TableExecutor &&) = default;
    TableExecutor &operator=(const TableExecutor &) = default;
    TableExecutor &operator=(TableExecutor &&) = default;
    virtual ~TableExecutor() = default;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_ */
