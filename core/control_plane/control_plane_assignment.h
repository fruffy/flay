#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ASSIGNMENT_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ASSIGNMENT_H_

#include <z3++.h>

#include "backends/p4tools/modules/flay/core/lib/z3_cache.h"
#include "ir/ir.h"
#include "ir/node.h"

namespace P4Tools::Flay {

/// The set of concrete mappings of symbolic control plane variables for table match keys.
/// TODO: Make this an unordered set.
using ControlPlaneAssignmentPointerPair =
    std::pair<const IR::SymbolicVariable *, const IR::Expression *>;
using ControlPlaneAssignmentReferencePair =
    std::pair<std::reference_wrapper<const IR::SymbolicVariable>,
              std::reference_wrapper<const IR::Expression>>;
struct IsSemanticallyLessPairComparator {
    bool operator()(const ControlPlaneAssignmentPointerPair &s1,
                    const ControlPlaneAssignmentPointerPair &s2) const {
        if (!s1.first->equiv(*s2.first)) {
            return s1.first->isSemanticallyLess(*s2.first);
        }
        return s1.second->isSemanticallyLess(*s2.second);
    }
    bool operator()(const ControlPlaneAssignmentReferencePair &s1,
                    const ControlPlaneAssignmentReferencePair &s2) const {
        if (!s1.first.get().equiv(s2.first)) {
            return s1.first.get().isSemanticallyLess(s2.first);
        }
        return s1.second.get().isSemanticallyLess(s2.second);
    }
};

using ControlPlaneAssignmentSet =
    ordered_map<std::reference_wrapper<const IR::SymbolicVariable>,
                std::reference_wrapper<const IR::Expression>, IR::IsSemanticallyLessComparator>;

/// Compares two control plane assignment sets. Returns true if s1 is shorter than s2. Also returns
/// true if a key in S1 is < S2's key or its value is < S2's value.
inline bool compare(const ControlPlaneAssignmentSet &s1, const ControlPlaneAssignmentSet &s2) {
    auto it = s2.begin();
    for (const auto &el : s1) {
        if (it == s2.end()) {
            return false;
        }
        if (el.first.get().isSemanticallyLess(it->first)) {
            return true;
        }
        if (it->first.get().isSemanticallyLess(el.first)) {
            return false;
        }
        if (el.second.get().isSemanticallyLess(it->second)) {
            return true;
        }
        if (it->second.get().isSemanticallyLess(el.second)) {
            return false;
        }
        ++it;
    }
    return it != s2.end();
}

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ASSIGNMENT_H_ */
