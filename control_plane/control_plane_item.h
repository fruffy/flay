#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_

#include <map>

#include "ir/ir.h"
#include "lib/castable.h"

namespace P4Tools::Flay {

/// A control plane item is any control plane construct that can influence program execution.
/// An example is a table entry executing a particular action and matching on a particular set of
/// keys.
class ControlPlaneItem : public ICastable {
 public:
    ControlPlaneItem() = default;
    ControlPlaneItem(const ControlPlaneItem &) = default;
    ControlPlaneItem(ControlPlaneItem &&) = default;
    ControlPlaneItem &operator=(const ControlPlaneItem &) = default;
    ControlPlaneItem &operator=(ControlPlaneItem &&) = default;
    ~ControlPlaneItem() override = default;

    virtual bool operator<(const ControlPlaneItem &other) const = 0;

    /// Generate an expression that represents the constraint imposed by the control plane item.
    [[nodiscard]] virtual const IR::Expression *computeControlPlaneConstraint() const = 0;
};

/// The constraints imposed by the control plane on the program. The map key is a unique identifier
/// of the object, typically its control plane name.
using ControlPlaneConstraints = std::map<cstring, std::reference_wrapper<ControlPlaneItem>>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_ */
