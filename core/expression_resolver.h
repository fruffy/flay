#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Simplifies an expression, executes method calls, and resolves state references.
class ExpressionResolver : public Inspector {
    friend TableExecutor;

 private:
    /// The result of the table execution. Typically, a table info file.
    const IR::Expression *result = nullptr;

    /// The program info of the target.
    std::reference_wrapper<const ProgramInfo> programInfo;

    /// The current execution state.
    std::reference_wrapper<ExecutionState> executionState;

    /// Visitor methods.
    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::Literal *lit) override;
    bool preorder(const IR::PathExpression *path) override;
    bool preorder(const IR::Member *member) override;
    bool preorder(const IR::Operation_Unary *op) override;
    bool preorder(const IR::Operation_Binary *op) override;
    bool preorder(const IR::Operation_Ternary *op) override;
    bool preorder(const IR::StructExpression *structExpr) override;
    bool preorder(const IR::MethodCallExpression *call) override;

 protected:
    /// @returns the current execution state.
    ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    const ProgramInfo &getProgramInfo() const;

    /// @returns Executes the target-specific table implementation and returns a result.
    virtual const IR::Expression *processTable(const IR::P4Table *table) = 0;

 public:
    explicit ExpressionResolver(const ProgramInfo &programInfo, ExecutionState &executionState);

    /// @returns the result of the execution of this visitor.
    /// Throws BUG if the result is a nullptr.
    const IR::Expression *getResult();
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_ */
