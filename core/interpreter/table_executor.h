#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_TABLE_EXECUTOR_H_

#include <functional>

#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/modules/flay/core/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "ir/ir.h"
#include "ir/vector.h"

namespace P4Tools::Flay {

/// Forward declaration of the calling expression resolver.
class ExpressionResolver;

using KeyMap = std::set<const TableMatchKey *>;

/// Executes a table and synthesizes control plane action parameters.
class TableExecutor {
 private:
    /// The table associated with this executor.
    std::reference_wrapper<const IR::P4Table> _table;

    /// The resolver that instantiated this table execution..
    std::reference_wrapper<ExpressionResolver> _resolver;

    /// The prefix associated with symbolic variables in this table. This is usually the table name.
    cstring _symbolicTablePrefix;

    /// Resolves the input key and ensures that all members of the key are pure symbolic.
    /// @returns the symbolic key.
    const IR::Key *resolveKey(const IR::Key *key) const;

    struct ReturnProperties {
        const IR::Expression *totalHitCondition;
        const IR::Expression *actionRun;
    };

    /// Handle the default action.
    void processDefaultAction() const;

    /// Process all the possible actions in the table for which we could insert an entry.
    /// ReferenceState is the initial state of the table cloned before the default action was
    /// executed.
    void processTableActionOptions(const TableUtils::TableProperties &tableProperties,
                                   const ExecutionState &referenceState,
                                   ReturnProperties &tableReturnProperties) const;

    /// Produce a single key match expression from a map of keys.
    static const IR::Expression *buildKeyMatches(cstring tablePrefix, const KeyMap &keyMap);

 protected:
    /// Computes a series of boolean conditions that must be true for the table to hit a particular
    /// action.
    [[nodiscard]] virtual KeyMap computeHitCondition(const IR::Key &key) const;

    /// The resolver that instantiated this table execution.
    [[nodiscard]] ExpressionResolver &resolver() const;

    /// Sets the symbolic table name. Usually the control-plane name.
    void setSymbolicTablePrefix(cstring name);

    /// @returns the symbolic table name
    [[nodiscard]] cstring symbolicTablePrefix() const;

    /// @returns the table associated with this executor.
    [[nodiscard]] const IR::P4Table &getP4Table() const;

    /// @returns the current execution state.
    [[nodiscard]] ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    [[nodiscard]] const ProgramInfo &getProgramInfo() const;

    /// @returns the program info associated with the current target.
    [[nodiscard]] ControlPlaneConstraints &controlPlaneConstraints() const;

    /// Compute a table key match. Can be overridden by targets to add more match types.
    /// @returns the full match expression for the key given the table match types.
    virtual const TableMatchKey *computeTargetMatchType(const IR::KeyElement *keyField) const;

 public:
    /// Execute the table and @return the state after executing it (hit, action_run).
    const IR::Expression *processTable();

    /// Helper function to call an action with arguments.
    static void callAction(const ProgramInfo &programInfo,
                           ControlPlaneConstraints &controlPlaneConstraints, ExecutionState &state,
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

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_TABLE_EXECUTOR_H_ */
