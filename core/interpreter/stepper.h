#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_STEPPER_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/interpreter/execution_state.h"
#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4::P4Tools::Flay {

class ParserStepper;

class FlayStepper : public Inspector {
    /// The parser stepper should be able to access functions internal to the core stepper.
    friend ParserStepper;

    /// The program info of the target.
    std::reference_wrapper<const ProgramInfo> _programInfo;

    /// The control plane constraints of the current P4 program.
    std::reference_wrapper<ControlPlaneConstraints> _controlPlaneConstraints;

    /// The current execution state.
    std::reference_wrapper<ExecutionState> _executionState;

 protected:
    /// Called after the copy-in step in the parser.
    /// Can initialize some target-specific state before the rest of the parser is invoked.
    virtual void initializeParserState(const IR::P4Parser &parser);

    /// Called after the copy-in step in the control.
    /// Can initialize some target-specific state before the rest of the control is invoked.
    virtual void initializeControlState(const IR::P4Control &control);

    /// Visitor methods.
    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Parser *parser) override;
    bool preorder(const IR::AssignmentStatement *assign) override;
    bool preorder(const IR::EmptyStatement *emptyStatement) override;
    bool preorder(const IR::BlockStatement *block) override;
    bool preorder(const IR::IfStatement *ifStatement) override;
    bool preorder(const IR::SwitchStatement *switchStatement) override;
    bool preorder(const IR::MethodCallStatement *callStatement) override;

    /// @returns the current execution state.
    ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    virtual const ProgramInfo &getProgramInfo() const;

    /// @returns the control plane constraints associated with the current program.
    virtual ControlPlaneConstraints &controlPlaneConstraints() const;

    /// @returns the expression resolver associated with this stepper. And with that the expression
    /// resolver associated with this target. Targets may implement custom externs and table
    /// definitions, which are modeled within the expression resolver.
    virtual ExpressionResolver &createExpressionResolver() const = 0;

 public:
    /// Pre-run initialization method. Every stepper should implement this.
    /// TODO: Replace with init_apply?
    virtual void initializeState() = 0;

    explicit FlayStepper(const ProgramInfo &programInfo, ControlPlaneConstraints &constraints,
                         ExecutionState &executionState);
};

}  // namespace P4::P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_STEPPER_H_ */
