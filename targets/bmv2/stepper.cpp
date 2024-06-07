#include "backends/p4tools/modules/flay/targets/bmv2/stepper.h"

#include <cstddef>
#include <utility>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/targets/bmv2/constants.h"
#include "ir/irutils.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay::V1Model {

const Bmv2V1ModelProgramInfo &V1ModelFlayStepper::getProgramInfo() const {
    return *FlayStepper::getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
}

void V1ModelFlayStepper::initializeState() {
    const auto &target = FlayTarget::get();
    const auto *archSpec = FlayTarget::getArchSpec();
    const auto *programmableBlocks = getProgramInfo().getProgrammableBlocks();
    auto &executionState = getExecutionState();
    // BMv2 initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = archSpec->getArchMember(blockIdx);
        executionState.initializeBlockParams(target, typeDecl, &archMember->blockParams);
        blockIdx++;
    }

    const auto *thirtyTwoBitType = IR::Type_Bits::get(32);

    // Initialize instance_type with a place holder.
    const auto *instanceTypeVar = new IR::Member(
        thirtyTwoBitType, new IR::PathExpression("*standard_metadata"), "instance_type");
    const IR::Expression *instanceTypeValue = nullptr;
    if (FlayOptions::get().usePlaceholders()) {
        instanceTypeValue = new IR::Placeholder(
            cstring("standard_metadata.instance_type"),
            IR::Constant::get(thirtyTwoBitType, V1ModelConstants::PKT_INSTANCE_TYPE_NORMAL));
    } else {
        instanceTypeValue =
            new IR::SymbolicVariable(thirtyTwoBitType, cstring("standard_metadata.instance_type"));
    }
    executionState.set(instanceTypeVar, instanceTypeValue);
    executionState.setPlaceholderValue(
        cstring("standard_metadata.instance_type"),
        IR::Constant::get(thirtyTwoBitType, V1ModelConstants::PKT_INSTANCE_TYPE_NORMAL));
}

void V1ModelFlayStepper::initializeParserState(const IR::P4Parser &parser) {
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

V1ModelFlayStepper::V1ModelFlayStepper(const V1Model::Bmv2V1ModelProgramInfo &programInfo,
                                       ExecutionState &executionState)
    : FlayStepper(programInfo, executionState) {}

V1ModelExpressionResolver &V1ModelFlayStepper::createExpressionResolver(
    const ProgramInfo &programInfo, ExecutionState &executionState) const {
    return *new V1ModelExpressionResolver(programInfo, executionState);
}

}  // namespace P4Tools::Flay::V1Model
