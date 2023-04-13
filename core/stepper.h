#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class FlayStepper : public Inspector {
 private:
    std::reference_wrapper<const ProgramInfo> programInfo;

    std::reference_wrapper<ExecutionState> executionState;

 protected:
    ExecutionState &getExecutionState() const;

    virtual const ProgramInfo &getProgramInfo() const;

    static void declareStructLike(ExecutionState &nextState, const IR::Expression *parentExpr,
                                  const IR::Type_StructLike *structType, bool forceTaint);

    void initializeBlockParams(const IR::Type_Declaration *typeDecl,
                               const std::vector<cstring> *blockParams,
                               ExecutionState &nextState) const;

 public:
    virtual void initializeState() = 0;

    explicit FlayStepper(const ProgramInfo &programInfo, ExecutionState &executionState);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_ */
