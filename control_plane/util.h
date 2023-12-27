#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_
#include <functional>

#include "ir/ir.h"

namespace P4Tools::Flay {

/// Utility function to compare IR nodes in a set. We use their source info.
struct SourceIdCmp {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const {
        return s1->srcInfo < s2->srcInfo;
    }
};

/// Data structures which simplify the handling of symbolic variables.
using SymbolSet = std::set<std::reference_wrapper<const IR::SymbolicVariable>,
                           std::less<const IR::SymbolicVariable &>>;
using SymbolMap = std::map<std::reference_wrapper<const IR::SymbolicVariable>,
                           std::set<const IR::Node *>, std::less<const IR::SymbolicVariable &>>;
using NodeSet = std::set<const IR::Node *, SourceIdCmp>;

/// Collects all IR::SymbolicVariables in a given IR::Node.
class SymbolCollector : public Inspector {
 private:
    /// The set of symbolic variables.
    SymbolSet collectedSymbols;

 public:
    SymbolCollector() = default;

    void postorder(const IR::SymbolicVariable *symbolicVariable) override {
        collectedSymbols.insert(*symbolicVariable);
    }

    /// Returns the collected  symbolic variables.
    [[nodiscard]] const SymbolSet &getCollectedSymbols() const { return collectedSymbols; }
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_ */
