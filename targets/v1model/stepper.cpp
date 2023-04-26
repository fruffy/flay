#include "backends/p4tools/modules/flay/targets/v1model/stepper.h"

#include <cstddef>
#include <list>
#include <utility>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/state_utils.h"
#include "backends/p4tools/modules/flay/core/target.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

namespace P4Tools::Flay::V1Model {

const V1ModelProgramInfo &V1ModelFlayStepper::getProgramInfo() const {
    return *FlayStepper::getProgramInfo().checkedTo<V1ModelProgramInfo>();
}

void V1ModelFlayStepper::initializeState() {
    const auto *archSpec = FlayTarget::getArchSpec();
    const auto &programInfo = getProgramInfo();
    const auto *programmableBlocks = getProgramInfo().getProgrammableBlocks();
    auto &executionState = getExecutionState();
    // BMv2 initializes all metadata to zero. To avoid unnecessary taint, we retrieve the type and
    // initialize all the relevant metadata variables to zero.
    size_t blockIdx = 0;
    for (const auto &blockTuple : *programmableBlocks) {
        const auto *typeDecl = blockTuple.second;
        const auto *archMember = archSpec->getArchMember(blockIdx);
        StateUtils::initializeBlockParams(programInfo, executionState, typeDecl,
                                          &archMember->blockParams);
        blockIdx++;
    }
}

V1ModelFlayStepper::V1ModelFlayStepper(const V1Model::V1ModelProgramInfo &programInfo,
                                       ExecutionState &executionState)
    : FlayStepper(programInfo, executionState) {}

V1ModelExpressionResolver &V1ModelFlayStepper::createExpressionResolver(
    const ProgramInfo &programInfo, ExecutionState &executionState) const {
    return *new V1ModelExpressionResolver(programInfo, executionState);
};

}  // namespace P4Tools::Flay::V1Model
