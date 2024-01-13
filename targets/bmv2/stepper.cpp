#include "backends/p4tools/modules/flay/targets/bmv2/stepper.h"

#include <cstddef>
#include <utility>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "backends/p4tools/modules/flay/targets/bmv2/constants.h"
#include "ir/irutils.h"

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

    const auto *thirtyTwoBitType = IR::getBitType(32);

    // Initialize instance_type with a place holder.
    const auto *instanceTypeVar = new IR::Member(
        thirtyTwoBitType, new IR::PathExpression("*standard_metadata"), "instance_type");
    executionState.set(
        instanceTypeVar,
        new IR::Placeholder(
            "standard_metadata.instance_type",
            IR::getConstant(thirtyTwoBitType, V1ModelConstants::PKT_INSTANCE_TYPE_NORMAL)));
    executionState.setPlaceholderValue(
        "standard_metadata.instance_type",
        IR::getConstant(thirtyTwoBitType, V1ModelConstants::PKT_INSTANCE_TYPE_NORMAL));
}

V1ModelFlayStepper::V1ModelFlayStepper(const V1Model::Bmv2V1ModelProgramInfo &programInfo,
                                       ExecutionState &executionState)
    : FlayStepper(programInfo, executionState) {}

V1ModelExpressionResolver &V1ModelFlayStepper::createExpressionResolver(
    const ProgramInfo &programInfo, ExecutionState &executionState) const {
    return *new V1ModelExpressionResolver(programInfo, executionState);
}

}  // namespace P4Tools::Flay::V1Model
