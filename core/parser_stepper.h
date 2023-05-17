#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PARSER_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PARSER_STEPPER_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/stepper.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class ParserStepper : public Inspector {
 public:
    /// A parser exit state is a state in which the parser has finished parsing.
    /// This could either be a reject or an accept.
    using ParserExitState = std::reference_wrapper<const ExecutionState>;

 private:
    /// Reference to the main stepper calling this parser stepper.
    std::reference_wrapper<FlayStepper> stepper;

    /// The list of parser exit states associated with this parser.
    std::vector<ParserExitState> parserExitStates;

    /// Add an exit state to the parser.
    void addParserExitState(const ExecutionState &state);

    void processSelectExpression(const IR::SelectExpression *selectExpr);

    /// Visitor methods.
    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::P4Parser *parser) override;
    bool preorder(const IR::ParserState *parserState) override;

 protected:
    /// @returns the current execution state.
    ExecutionState &getExecutionState() const;

    /// @returns the program info associated with the current target.
    virtual const ProgramInfo &getProgramInfo() const;

 public:
    explicit ParserStepper(FlayStepper &stepper);

    /// @returns the parser exit states associated with this parser stepper.
    const std::vector<ParserExitState> &getParserExitStates() const;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_PARSER_STEPPER_H_ */
