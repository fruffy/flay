#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ASSIGNMENT_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ASSIGNMENT_H_

#include <z3++.h>

#include <algorithm>
#include <map>

#include "backends/p4tools/modules/flay/core/lib/z3_cache.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/castable.h"

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

class Z3ControlPlaneAssignmentSet
    : private ordered_map<std::reference_wrapper<const IR::SymbolicVariable>, z3::expr,
                          IR::IsSemanticallyLessComparator> {
 public:
    Z3ControlPlaneAssignmentSet() = default;

    void add(const IR::SymbolicVariable &var, z3::expr assignment) {
        auto result = emplace(var, assignment);
        if (!result.second) {
            ::error("Duplicate Z3ControlPlaneAssignmentSet entry");
        }
    }

    void addConditionally(const IR::SymbolicVariable &var, const z3::expr &condition,
                          z3::expr assignment) {
        auto it = find(var);
        if (it == end()) {
            emplace(var, assignment);
        } else {
            it->second = z3::ite(condition, assignment, it->second);
        }
    }

    [[nodiscard]] z3::expr substitute(z3::expr toSubstitute) const {
        z3::expr_vector substitutionVariables(toSubstitute.ctx());
        z3::expr_vector substitutionAssignments(toSubstitute.ctx());
        for (const auto &match : *this) {
            substitutionVariables.push_back(Z3Cache::set(&match.first.get()));
            substitutionAssignments.push_back(match.second);
        }
        return toSubstitute.substitute(substitutionVariables, substitutionAssignments);
    }

    void merge(const Z3ControlPlaneAssignmentSet &other) {
        for (const auto &match : other) {
            add(match.first.get(), match.second);
        }
    }

    void mergeConditionally(const z3::expr &condition, const Z3ControlPlaneAssignmentSet &other) {
        for (const auto &match : other) {
            addConditionally(match.first.get(), condition, match.second);
        }
    }
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ASSIGNMENT_H_ */
