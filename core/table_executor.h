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

    /// The resolver that instantiated this table execution..
    std::reference_wrapper<ExpressionResolver> resolver;

    /// Resolves the input key and ensures that all members of the key are pure symbolic.
    /// @returns the symbolic key.
    const IR::Key *resolveKey(const IR::Key *key) const;

    void processDefaultAction(const IR::P4Table *table) const;

    void processTableActionOptions(const IR::P4Table *table,
                                   const IR::SymbolicVariable *tableActionID,
                                   const IR::Key *key) const;

 protected:
    /// @returns the current execution state.
    [[nodiscard]] ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    [[nodiscard]] const ProgramInfo &getProgramInfo() const;

 public:
    const IR::Expression *processTable(const IR::P4Table *table);

    explicit TableExecutor(ExpressionResolver &callingResolver);
    TableExecutor(const TableExecutor &) = default;
    TableExecutor(TableExecutor &&) = default;
    TableExecutor &operator=(const TableExecutor &) = default;
    TableExecutor &operator=(TableExecutor &&) = default;
    virtual ~TableExecutor() = default;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_ */
