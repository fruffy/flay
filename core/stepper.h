#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

class FlayStepper : public Inspector {
 private:
    std::reference_wrapper<const ProgramInfo> programInfo;

    std::reference_wrapper<ExecutionState> executionState;

    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::P4Control *p4control) override;
    bool preorder(const IR::AssignmentStatement *assign) override;
    bool preorder(const IR::BlockStatement *block) override;
    bool preorder(const IR::IfStatement *ifStmt) override;
    bool preorder(const IR::MethodCallStatement *stmt) override;

 protected:
    ExecutionState &getExecutionState() const;

    virtual const ProgramInfo &getProgramInfo() const;

 public:
    virtual void initializeState() = 0;

    explicit FlayStepper(const ProgramInfo &programInfo, ExecutionState &executionState);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STEPPER_H_ */
