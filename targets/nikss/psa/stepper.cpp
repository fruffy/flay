#include "backends/p4tools/modules/flay/targets/nikss/psa/stepper.h"

#include <utility>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"

namespace P4::P4Tools::Flay::Nikss {

const PsaProgramInfo &PsaFlayStepper::getProgramInfo() const {
    return *NikssBaseFlayStepper::getProgramInfo().checkedTo<PsaProgramInfo>();
}

PsaFlayStepper::PsaFlayStepper(const PsaProgramInfo &programInfo,
                               ControlPlaneConstraints &constraints, ExecutionState &executionState)
    : NikssBaseFlayStepper(programInfo, constraints, executionState) {}

PsaExpressionResolver &PsaFlayStepper::createExpressionResolver() const {
    return *new PsaExpressionResolver(getProgramInfo(), controlPlaneConstraints(),
                                      getExecutionState());
}

void PsaFlayStepper::initializeParserState(const IR::P4Parser &parser) {
    BUG_CHECK(parser.getApplyParameters()->parameters.size() == 4,
              "Expected 4 parameters for parser %1%", parser);
    // Initialize the parser error to be a symbolic variable.
    const auto *metadataBlock = parser.getApplyParameters()->parameters.at(3);
    const auto *thirtyTwoBitType = IR::Type_Bits::get(32);
    auto *parserErrorVariable = new IR::Member(
        thirtyTwoBitType, new IR::PathExpression(metadataBlock->name), "parser_error");
    auto parserErrorLabel =
        parser.controlPlaneName() + "_" + metadataBlock->controlPlaneName() + "_parser_error";
    getExecutionState().set(parserErrorVariable, ToolsVariables::getSymbolicVariable(
                                                     thirtyTwoBitType, parserErrorLabel));
}

}  // namespace P4::P4Tools::Flay::Nikss
