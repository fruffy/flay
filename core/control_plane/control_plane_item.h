#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_

#include <z3++.h>

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
    std::map<std::reference_wrapper<const IR::SymbolicVariable>,
             std::reference_wrapper<const IR::Expression>, IR::IsSemanticallyLessComparator>;

inline const IR::Expression *computeConstraintExpression(
    const ControlPlaneAssignmentSet &constraints) {
    if (constraints.size() == 0) {
        return IR::BoolLiteral::get(false);
    }
    const IR::Expression *matchExpression = nullptr;
    // Precompute the match expression in the constructor.
    for (const auto &match : constraints) {
        const auto &symbolicVariable = match.first.get();
        const auto &assignment = match.second.get();
        if (matchExpression == nullptr) {
            matchExpression = new IR::Equ(&symbolicVariable, &assignment);
        } else {
            matchExpression =
                new IR::LAnd(matchExpression, new IR::Equ(&symbolicVariable, &assignment));
        }
    }
    return matchExpression;
}

class Z3ControlPlaneAssignmentSet
    : private std::map<std::reference_wrapper<const IR::SymbolicVariable>, z3::expr,
                       IR::IsSemanticallyLessComparator> {
 public:
    Z3ControlPlaneAssignmentSet() = default;

    // [[nodiscard]] z3::expr_vector symbolicVariables() const { return _symbolicVariables; }

    // [[nodiscard]] z3::expr_vector assignments() const { return _assignments; }

    void add(const IR::SymbolicVariable &var, z3::expr assignment) { emplace(var, assignment); }

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

/// A control plane item is any control plane construct that can influence program execution.
/// An example is a table entry executing a particular action and matching on a particular set
/// of keys.
class ControlPlaneItem : public ICastable {
 public:
    ControlPlaneItem() = default;
    ControlPlaneItem(const ControlPlaneItem &) = default;
    ControlPlaneItem(ControlPlaneItem &&) = default;
    ControlPlaneItem &operator=(const ControlPlaneItem &) = default;
    ControlPlaneItem &operator=(ControlPlaneItem &&) = default;
    ~ControlPlaneItem() override = default;

    virtual bool operator<(const ControlPlaneItem &other) const = 0;

    /// Get the control plane assignments produced by the control plane item.
    [[nodiscard]] virtual ControlPlaneAssignmentSet computeControlPlaneAssignments() const = 0;

    /// Get the control plane constraints produced by the control plane item.
    [[nodiscard]] virtual Z3ControlPlaneAssignmentSet computeZ3ControlPlaneAssignments() const = 0;
};

/// The constraints imposed by the control plane on the program. The map key is a unique
/// identifier of the object, typically its control plane name.
using ControlPlaneConstraints = std::map<cstring, std::reference_wrapper<ControlPlaneItem>>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_CONTROL_PLANE_CONTROL_PLANE_ITEM_H_ */
