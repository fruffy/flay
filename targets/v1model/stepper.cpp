#include "backends/p4tools/modules/flay/targets/v1model/stepper.h"

#include "backends/p4tools/modules/flay/core/target.h"

namespace P4Tools::Flay::V1Model {

const V1ModelProgramInfo &V1ModelFlayStepper::getProgramInfo() const {
    return *FlayStepper::getProgramInfo().checkedTo<V1ModelProgramInfo>();
}

void V1ModelFlayStepper::initializeState() {
    const auto *archSpec = FlayTarget::getArchSpec();
    const auto *programmableBlocks = getProgramInfo().getProgrammableBlocks();

    // BMv2 initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = archSpec->getArchMember(blockIdx);
        initializeBlockParams(typeDecl, &archMember->blockParams, getExecutionState(), false);
        blockIdx++;
    }
}

bool V1ModelFlayStepper::preorder(const IR::Node *node) {
    printf("VISITING\n");
    node->dbprint(std::cout);
    printf("\n");
    return false;
}

V1ModelFlayStepper::V1ModelFlayStepper(const V1Model::V1ModelProgramInfo &programInfo,
                                       ExecutionState &executionState)
    : FlayStepper(programInfo, executionState) {}

}  // namespace P4Tools::Flay::V1Model
