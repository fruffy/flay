#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_NODE_MAP_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_NODE_MAP_H_

#include "backends/p4tools/modules/flay/core/interpreter/reachability_expression.h"
#include "backends/p4tools/modules/flay/core/interpreter/substitution_expression.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4Tools::Flay {

/// Annotates P4C nodes with specific information (e.g., reachability or the present value).
class NodeAnnotationMap {
 private:
    /// Associated reachability information with a particular node.
    ReachabilityMap _reachabilityMap;

    /// A mapping of expressions to their values in the node annotation map.
    SubstitutionMap _substitutionMap;

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

    /// Initialize the expression mapping for the given node.
    /// @returns false if the node is already mapped.
    bool initializeExpressionMapping(const IR::Expression *expression, const IR::Expression *value,
                                     const IR::Expression *cond);

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
    [[nodiscard]] SubstitutionMap substitutionMap() const;
};

}  // namespace P4Tools::Flay
#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_INTERPRETER_NODE_MAP_H_ */
