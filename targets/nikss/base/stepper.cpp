#include "backends/p4tools/modules/flay/targets/nikss/base/stepper.h"

#include <cstddef>
#include <utility>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/modules/flay/core/interpreter/program_info.h"
#include "backends/p4tools/modules/flay/core/interpreter/target.h"

namespace P4::P4Tools::Flay::Nikss {

const NikssBaseProgramInfo &NikssBaseFlayStepper::getProgramInfo() const {
    return *FlayStepper::getProgramInfo().checkedTo<NikssBaseProgramInfo>();
}

void NikssBaseFlayStepper::initializeState() {
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

NikssBaseFlayStepper::NikssBaseFlayStepper(const NikssBaseProgramInfo &programInfo,
                                           ControlPlaneConstraints &constraints,
                                           ExecutionState &executionState)
    : FlayStepper(programInfo, constraints, executionState) {}

}  // namespace P4::P4Tools::Flay::Nikss
