#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

class FlayStepper : public Inspector {
 private:
    /// The program info of the target.
    std::reference_wrapper<const ProgramInfo> programInfo;

    /// The current execution state.
    std::reference_wrapper<ExecutionState> executionState;

    /// Visitor methods.
    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Parser *parser) override;
    bool preorder(const IR::ParserState *parserState) override;
    bool preorder(const IR::AssignmentStatement *assign) override;
    bool preorder(const IR::BlockStatement *block) override;
    bool preorder(const IR::IfStatement *ifStmt) override;
    bool preorder(const IR::MethodCallStatement *stmt) override;

 protected:
    /// @returns the current execution state.
    ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    virtual const ProgramInfo &getProgramInfo() const;

    /// @returns the expression resolver associated with this stepper. And with that the expression
    /// resolver associated with this target. Targets may implement custom externs and table
    /// definitions, which are modeled within the expression resolver.
    virtual ExpressionResolver &createExpressionResolver(const ProgramInfo &programInfo,
                                                         ExecutionState &executionState) const = 0;

 public:
    /// Pre-run initialization method. Every stepper should implement this.
    /// TODO: Replace with init_apply?
    virtual void initializeState() = 0;

    explicit FlayStepper(const ProgramInfo &programInfo, ExecutionState &executionState);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_ */
