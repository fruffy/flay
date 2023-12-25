#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/reachability.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

class ElimDeadCode : public Transform {
    /// The reachability map computed by the execution state.
    std::reference_wrapper<const ReachabilityMap> reachabilityMap;

 public:
    ElimDeadCode() = delete;

    explicit ElimDeadCode(const ReachabilityMap &reachabilityMap);

    const IR::Node *postorder(IR::IfStatement *stmt) override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_ */
