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

bool TofinoBaseFlayStepper::preorder(const IR::P4Control *control) {
    // Set the parser error to a symbolic variable before stepping into the control.
    // TODO: This is imprecise. We should be able to set the parser error to the right value in the
    // parser.
    const auto *sixteenBitType = IR::Type_Bits::get(16);
    {
        const auto *parserErrorVar = new IR::Member(
            sixteenBitType, new IR::PathExpression("*ig_intr_md_from_prsr"), "parser_err");
        const IR::Expression *parserErrorValue =
            new IR::SymbolicVariable(sixteenBitType, "ig_intr_md_from_prsr.parser_err");
        getExecutionState().set(parserErrorVar, parserErrorValue);
    }
    {
        const auto *parserErrorVar = new IR::Member(
            sixteenBitType, new IR::PathExpression("*eg_intr_md_from_prsr"), "parser_err");
        const IR::Expression *parserErrorValue =
            new IR::SymbolicVariable(sixteenBitType, "eg_intr_md_from_prsr.parser_err");
        getExecutionState().set(parserErrorVar, parserErrorValue);
    }
    return FlayStepper::preorder(control);
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
