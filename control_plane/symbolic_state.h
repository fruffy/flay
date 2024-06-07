#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_SYMBOLIC_STATE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_SYMBOLIC_STATE_H_

#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

/// Utility function to compare IR nodes in a set. We use their source info.
struct SourceIdCmp {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const {
        return s1->srcInfo < s2->srcInfo;
    }
};

/// Data structures which simplify the handling of symbolic variables.
using SymbolSet =
    std::set<std::reference_wrapper<const IR::SymbolicVariable>, IR::IsSemanticallyLessComparator>;
using SymbolMap = std::map<std::reference_wrapper<const IR::SymbolicVariable>,
                           std::set<const IR::Node *>, IR::IsSemanticallyLessComparator>;
using NodeSet = std::set<const IR::Node *, SourceIdCmp>;
using ExpressionSet = std::set<const IR::Expression *, SourceIdCmp>;

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

class ControlPlaneStateInitializer : public Inspector {
 protected:
    /// The default control-plane constraints as defined by a target.
    ControlPlaneConstraints defaultConstraints;

    /// The reference map to look up specific declarations.
    std::reference_wrapper<const P4::ReferenceMap> refMap_;

    /// Tries to assemble a match for the given entry key and inserts into the @param keySet.
    /// @returns false if the match type is not supported.
    virtual bool computeMatch(const IR::Expression &entryKey, const IR::SymbolicVariable &keySymbol,
                              cstring tableName, cstring fieldName, cstring matchType,
                              TableKeySet &keySet);

    /// Tries to build a TableKeySet by matching the table fields and the keys listed in the entry.
    /// @returns std::nullopt if this is not possible.
    std::optional<TableKeySet> computeEntryKeySet(const IR::P4Table &table, const IR::Entry &entry);

    /// Tries to build a table control plane entry from the P4 entry member of the table.
    /// @returns std::nullopt if an error occurs when doing this.
    std::optional<TableEntrySet> initializeTableEntries(const IR::P4Table *table,
                                                        const P4::ReferenceMap &refMap);

    /// Compute the constraints imposed by the default action if the table is not configured.
    /// @returns std::nullopt if an error occurs.
    static std::optional<const IR::Expression *> computeDefaultActionConstraints(
        const IR::P4Table *table, const P4::ReferenceMap &refMap);

 public:
    explicit ControlPlaneStateInitializer(const P4::ReferenceMap &refMap);

    virtual std::optional<ControlPlaneConstraints> generateInitialControlPlaneConstraints(
        const IR::P4Program *program) = 0;

    /// @returns the default control-plane constraints.
    [[nodiscard]] ControlPlaneConstraints getDefaultConstraints() const;

 private:
    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::P4ValueSet *parserValueSet) override;
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_SYMBOLIC_STATE_H_ */
