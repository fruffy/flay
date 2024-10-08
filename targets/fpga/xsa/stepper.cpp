#include "backends/p4tools/modules/flay/targets/fpga/xsa/stepper.h"

#include <utility>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"

namespace P4::P4Tools::Flay::Fpga {

const XsaProgramInfo &XsaFlayStepper::getProgramInfo() const {
    return *FpgaBaseFlayStepper::getProgramInfo().checkedTo<XsaProgramInfo>();
}

XsaFlayStepper::XsaFlayStepper(const XsaProgramInfo &programInfo,
                               ControlPlaneConstraints &constraints, ExecutionState &executionState)
    : FpgaBaseFlayStepper(programInfo, constraints, executionState) {}

XsaExpressionResolver &XsaFlayStepper::createExpressionResolver() const {
    return *new XsaExpressionResolver(getProgramInfo(), controlPlaneConstraints(),
                                      getExecutionState());
}

void XsaFlayStepper::initializeParserState(const IR::P4Parser &parser) {
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

}  // namespace P4::P4Tools::Flay::Fpga
