#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

/// Defines accessors and utility functions for state that is managed by the control plane.
/// This class can be extended by targets to customize initialization behavior and add
/// target-specific utility functions.
class ControlPlaneState : public ICastable {
 protected:
    /// The default control-plane constraints as defined by a target.
    ControlPlaneConstraints defaultConstraints;

 public:
    /// @returns the default control-plane constraints.
    ControlPlaneConstraints getDefaultConstraints() const;

    /// @returns the symbolic boolean variable describing whether a table is configured by the
    /// control plane.
    static const IR::SymbolicVariable *getTableActive(cstring tableName);

    /// @returns the symbolic variable describing a table match key.
    static const IR::SymbolicVariable *getTableKey(cstring tableName, cstring fieldName,
                                                   const IR::Type *type);

    /// @returns the symbolic variable describing a table ternary mask.
    static const IR::SymbolicVariable *getTableTernaryMask(cstring tableName, cstring fieldName,
                                                           const IR::Type *type);

    /// @returns the symbolic variable describing a table LPM prefix match
    static const IR::SymbolicVariable *getTableMatchLpmPrefix(cstring tableName, cstring fieldName,
                                                              const IR::Type *type);

    /// @returns the symbolic variable describing an action argument as part of a match-action call.
    static const IR::SymbolicVariable *getTableActionArg(cstring tableName, cstring actionName,
                                                         cstring parameterName,
                                                         const IR::Type *type);

    /// @returns the symbolic variable listing the chosen action for a particular table.
    static const IR::SymbolicVariable *getTableActionChoice(cstring tableName);

    /// Initializes the symbolic variable for a table and adds it to @defaultConstraints.
    virtual const IR::SymbolicVariable *allocateControlPlaneTable(cstring tableName) = 0;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_SYMBOLIC_STATE_H_ */
