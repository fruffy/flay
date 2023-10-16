#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_

#include <map>

#include "ir/ir.h"
#include "ir/solver.h"

namespace P4Tools::Flay {

using ControlPlaneConstraints = std::vector<const Constraint *>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_ */