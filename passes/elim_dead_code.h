#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_

#include <functional>

#include "backends/p4tools/modules/flay/core/reachability.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/createBuiltins.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// This compiler pass looks up program nodes in the reachability map and deletes nodes which are
/// not executable according to the computation in the map.
class ElimDeadCode : public Transform {
    /// The reachability map computed by the execution state.
    std::reference_wrapper<const ReachabilityMap> reachabilityMap;

    /// The precomputed reference map.
    std::reference_wrapper<const P4::ReferenceMap> refMap;

 public:
    ElimDeadCode() = delete;

    explicit ElimDeadCode(const P4::ReferenceMap &refMap, const ReachabilityMap &reachabilityMap);

    const IR::Node *preorder(IR::IfStatement *stmt) override;
    const IR::Node *preorder(IR::SwitchStatement *switchStmt) override;
    const IR::Node *preorder(IR::MethodCallStatement *stmt) override;
};

class ReferenceResolver : public PassManager {
 public:
    explicit ReferenceResolver(P4::ReferenceMap &refMap) {
        addPasses({
            new P4::CreateBuiltins(),
            new P4::ResolveReferences(&refMap),
        });
    }
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_PASSES_ELIM_DEAD_CODE_H_ */
