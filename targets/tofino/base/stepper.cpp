#include "backends/p4tools/modules/flay/targets/tofino/base/stepper.h"

#include <cstddef>
#include <utility>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "backends/p4tools/modules/flay/core/target.h"

namespace P4Tools::Flay::Tofino {

const TofinoBaseProgramInfo &TofinoBaseFlayStepper::getProgramInfo() const {
    return *FlayStepper::getProgramInfo().checkedTo<TofinoBaseProgramInfo>();
}

void TofinoBaseFlayStepper::initializeState() {
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
}

TofinoBaseFlayStepper::TofinoBaseFlayStepper(const TofinoBaseProgramInfo &programInfo,
                                             ExecutionState &executionState)
    : FlayStepper(programInfo, executionState) {}

}  // namespace P4Tools::Flay::Tofino
