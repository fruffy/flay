#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Executes a table and synthesizes control plane action parameters.
class TableExecutor : public Inspector {
 private:
    /// The result of the table execution. Typically, a table info file.
    const IR::Expression *result = nullptr;

    /// The program info of the target.
    std::reference_wrapper<const ProgramInfo> programInfo;

    /// The current execution state.
    std::reference_wrapper<ExecutionState> executionState;

    /// Resolves the input key and ensures that all members of the key are pure symbolic.
    /// @returns the symbolic key.
    const IR::Key *resolveKey(const IR::Key *key) const;

    static std::vector<const IR::ActionListElement *> buildTableActionList(
        const IR::P4Table *table);

    /// Visitor methods.
    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::P4Table *table) override;

 protected:
    /// @returns the current execution state.
    ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    virtual const ProgramInfo &getProgramInfo() const;

 public:
    /// @returns the result of the execution of this visitor.
    /// Throws BUG if the result is a nullptr.
    const IR::Expression *getResult();

    explicit TableExecutor(const ProgramInfo &programInfo, ExecutionState &executionState);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_TABLE_EXECUTOR_H_ */
