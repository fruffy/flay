#include "backends/p4tools/modules/flay/targets/tofino/tofino1/stepper.h"

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"

namespace P4::P4Tools::Flay::Tofino {

const Tofino1ProgramInfo &Tofino1FlayStepper::getProgramInfo() const {
    return *TofinoBaseFlayStepper::getProgramInfo().checkedTo<Tofino1ProgramInfo>();
}

Tofino1FlayStepper::Tofino1FlayStepper(const Tofino1ProgramInfo &programInfo,
                                       ControlPlaneConstraints &constraints,
                                       ExecutionState &executionState)
    : TofinoBaseFlayStepper(programInfo, constraints, executionState) {}

Tofino1ExpressionResolver &Tofino1FlayStepper::createExpressionResolver() const {
    return *new Tofino1ExpressionResolver(getProgramInfo(), controlPlaneConstraints(),
                                          getExecutionState());
}

void Tofino1FlayStepper::initializeParserState(const IR::P4Parser &parser) {
    const auto &parameters = parser.getApplyParameters()->parameters;

    auto canonicalBlockName = getProgramInfo().getCanonicalBlockName(parser.name);
    const auto *sixteenBitType = IR::Type_Bits::get(16);
    if (canonicalBlockName == "IngressParserT") {
        // If the parser has fewer than 4 parameters, which means it does not set the parser error
        // within the parser block, we set the parser error directly in the global metadata.
        constexpr int kParserMetadataIndex = 3;
        if (parameters.size() > kParserMetadataIndex) {
            // Initialize the parser error to be a symbolic variable.
            const auto *metadataBlock =
                parser.getApplyParameters()->parameters.at(kParserMetadataIndex);
            auto *parserErrorVariable = new IR::Member(
                sixteenBitType, new IR::PathExpression(metadataBlock->name), "parser_err");
            auto parserErrorLabel =
                parser.controlPlaneName() + "_" + metadataBlock->controlPlaneName() + "_parser_err";
            getExecutionState().set(parserErrorVariable, ToolsVariables::getSymbolicVariable(
                                                             sixteenBitType, parserErrorLabel));
        }
    } else if (canonicalBlockName == "EgressParserT") {
        constexpr int kParserMetadataIndex = 4;
        // If the parser has fewer than 5 parameters, which means it does not set the parser error
        // within the parser block, we set the parser error directly in the global metadata.
        if (parameters.size() > kParserMetadataIndex) {
            // Initialize the parser error to be a symbolic variable.
            const auto *metadataBlock =
                parser.getApplyParameters()->parameters.at(kParserMetadataIndex);
            auto *parserErrorVariable = new IR::Member(
                sixteenBitType, new IR::PathExpression(metadataBlock->name), "parser_err");
            auto parserErrorLabel =
                parser.controlPlaneName() + "_" + metadataBlock->controlPlaneName() + "_parser_err";
            getExecutionState().set(parserErrorVariable, ToolsVariables::getSymbolicVariable(
                                                             sixteenBitType, parserErrorLabel));
        }
    } else {
        BUG("Unexpected canonical block name %1% for parser %2%", canonicalBlockName, parser);
    }
}

}  // namespace P4::P4Tools::Flay::Tofino
