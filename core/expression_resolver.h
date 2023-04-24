#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_

#include <functional>
#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

class ExpressionResolver : public Inspector {
 private:
    const IR::Expression *result = nullptr;

    std::reference_wrapper<const ProgramInfo> programInfo;

    std::reference_wrapper<ExecutionState> executionState;

    bool preorder(const IR::Node *node) override;
    bool preorder(const IR::Operation_Binary *op) override;
    bool preorder(const IR::Member *member) override;
    bool preorder(const IR::PathExpression *path) override;
    bool preorder(const IR::MethodCallExpression *call) override;

 protected:
    ExecutionState &getExecutionState() const;

    virtual const ProgramInfo &getProgramInfo() const;

 public:
    const IR::Expression *getResult();

    explicit ExpressionResolver(const ProgramInfo &programInfo, ExecutionState &executionState);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXPRESSION_RESOLVER_H_ */
