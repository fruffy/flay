#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_NODE_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_NODE_MAP_H_

#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"
#include "backends/p4tools/modules/flay/core/reachability_expression.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// Maps an expression with validation source information to its value as collected in the Flay
/// analysis.
using ExpressionMap = std::map<const IR::Expression *, const IR::Expression *, SourceIdCmp>;

/// Annotates P4C nodes with specific information (e.g., reachability or the present value).
class NodeAnnotationMap {
 private:
    /// Associated reachability information with a particular node.
    ReachabilityMap _reachabilityMap;

    /// A mapping of expressions to their values in the node annotation map.
    ExpressionMap _expressionMap;

    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// node annotation map. This map can we used for incremental re-computation.
    SymbolMap _expressionSymbolMap;

    /// A mapping of symbolic variables to IR nodes that depend on these symbolic variables in the
    /// node annotation map. This map can we used for incremental re-computation.
    SymbolMap _reachabilitySymbolMap;

 public:
    /// Initialize the reachability mapping for the given node.
    /// @returns false if the node is already mapped.
    bool initializeReachabilityMapping(const IR::Node *node, const IR::Expression *cond);

    bool initializeExpressionMapping(const IR::Expression *expression, const IR::Expression *value);

    /// Merge an other node annotation map into this node annotation map.
    void mergeAnnotationMapping(const NodeAnnotationMap &otherMap);

    /// Substitute all placeholders in the node annotation map and update each condition.
    void substitutePlaceholders(Transform &substitute);

    /// @returns the reachability symbol map accumulated in the map.
    [[nodiscard]] SymbolMap reachabilitySymbolMap() const;

    /// @returns the expression symbol map accumulated in the map.
    [[nodiscard]] SymbolMap expressionSymbolMap() const;

    /// @returns the reachability map associated with the node annotation map.
    [[nodiscard]] ReachabilityMap reachabilityMap() const;

    /// @returns the expression map associated with the node annotation map.
    [[nodiscard]] ExpressionMap expressionMap() const;
};

}  // namespace P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_NODE_MAP_H_ */
