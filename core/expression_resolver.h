#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/table_executor.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "ir/visitor.h"

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
    /// Mux is an Operation_Ternary, which can have side effects.
    bool preorder(const IR::Mux *mux) override;
    bool preorder(const IR::ListExpression *listExpr) override;
    bool preorder(const IR::StructExpression *structExpr) override;
    bool preorder(const IR::MethodCallExpression *call) override;

 protected:
    /// @returns the current execution state.
    ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    const ProgramInfo &getProgramInfo() const;

    /// Executes the target-specific table implementation and @returns the result of the execution.
    virtual const IR::Expression *processTable(const IR::P4Table *table) = 0;

    /// Tries to look up the implementation of the extern in the list of available extern functions
    /// for the expression resolver of the target. Returns the result of the execution.
    virtual const IR::Expression *processExtern(const IR::PathExpression &externObjectRef,
                                                const IR::ID &methodName,
                                                const IR::Vector<IR::Argument> *args);

    /// @returns the result of the execution of this visitor.
    /// Throws BUG if the result is a nullptr.
    const IR::Expression *getResult();

 public:
    explicit ExpressionResolver(const ProgramInfo &programInfo, ExecutionState &executionState);

    /// Apply the resolver to the supplied @param node.
    /// @returns the result of the execution of this visitor.
    /// Throws BUG if the result is a nullptr.
    const IR::Expression *computeResult(const IR::Node *node);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_ */
