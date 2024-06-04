#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_NODE_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_NODE_MAP_H_

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/core/reachability_expression.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// Annotates P4C nodes with specific information (e.g., reachability or the present value).
class NodeAnnotationMap {
 private:
    ReachabilityMap _reachabilityMap;

    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// node annotation map. This map can we used for incremental re-computation.
    SymbolMap _symbolMap;

 public:
    /// Initialize the reachability mapping for the given node.
    /// @returns false if the node is already mapped.
    bool initializeReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    /// Merge an other node annotation map into this node annotation map.
    void mergeAnnotationMapping(const NodeAnnotationMap &otherMap);

    /// Substitute all placeholders in the node annotation map and update each condition.
    void substitutePlaceholders(Transform &substitute);

    /// @returns the symbol map accumulated in the map.
    [[nodiscard]] SymbolMap symbolMap() const;

    /// @returns the reachability map associated with the node annotation map.
    [[nodiscard]] ReachabilityMap reachabilityMap() const;
};

}  // namespace P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_NODE_MAP_H_ */
