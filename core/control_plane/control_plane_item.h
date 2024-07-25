#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_

#include <map>

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_assignment.h"
#include "backends/p4tools/modules/flay/core/control_plane/z3_control_plane_assignment.h"
#include "lib/castable.h"

namespace P4Tools::Flay {

/// A control plane item is any control plane construct that can influence program execution.
/// An example is a table entry executing a particular action and matching on a particular set
/// of keys.
class ControlPlaneItem : public ICastable {
 public:
    virtual bool operator<(const ControlPlaneItem &other) const = 0;

    /// Get the control plane assignments produced by the control plane item.
    [[nodiscard]] virtual ControlPlaneAssignmentSet computeControlPlaneAssignments() const = 0;
};

class Z3ControlPlaneItem : public ControlPlaneItem {
 public:
    /// Get the control plane constraints produced by the control plane item.
    [[nodiscard]] virtual Z3ControlPlaneAssignmentSet computeZ3ControlPlaneAssignments() const = 0;
};

/// The constraints imposed by the control plane on the program. The map key is a unique
/// identifier of the object, typically its control plane name.
using ControlPlaneConstraints = std::map<cstring, std::reference_wrapper<Z3ControlPlaneItem>>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_ */
