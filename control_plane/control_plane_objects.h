#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_CONTROL_PLANE_OBJECTS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_CONTROL_PLANE_OBJECTS_H_

#include <cstdint>
#include <functional>
#include <set>
#include <utility>

#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4Tools::ControlPlaneState {

/// @returns the symbolic boolean variable indicating whether this particular parser value set has
/// been configured by the control plane.
const IR::SymbolicVariable *getParserValueSetConfigured(cstring parserValueSetName);

/// @returns the symbolic string variable that represents the default action that is active for a
/// particular table.
const IR::SymbolicVariable *getDefaultActionVariable(cstring tableName);

}  // namespace P4Tools::ControlPlaneState

namespace P4Tools::Flay {

/**************************************************************************************************
TableMatchEntry
**************************************************************************************************/

class TableMatchEntry : public ControlPlaneItem {
 protected:
    /// The action that will be executed by this entry.
    ControlPlaneAssignmentSet _actionAssignment;

    /// The priority of this entry.
    int32_t _priority;

    /// The expression which needs to be true to execute the action.
    const IR::Expression *_matchExpression;

    /// The resulting assignment once the match expression is true.
    const IR::Expression *_actionAssignmentExpression;

    /// The set of control plane assignments produced by this entry.
    ControlPlaneAssignmentSet _matches;

 public:
    explicit TableMatchEntry(ControlPlaneAssignmentSet actionAssignment, int32_t priority,
                             const ControlPlaneAssignmentSet &matches);

    /// @returns the action that will be executed by this entry.
    [[nodiscard]] ControlPlaneAssignmentSet actionAssignment() const;

    /// @returns the expression correlating to the action that will be executed by this entry.
    const IR::Expression *actionAssignmentExpression() const;

    /// @returns the priority of this entry.
    [[nodiscard]] int32_t priority() const;

    bool operator<(const ControlPlaneItem &other) const override;

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(TableMatchEntry);
};

/**************************************************************************************************
TableDefaultAction
**************************************************************************************************/

class TableDefaultAction : public ControlPlaneItem {
    /// The action that will be executed by this entry.
    ControlPlaneAssignmentSet _actionAssignment;

    /// The resulting assignment once the match expression is true.
    const IR::Expression *_actionAssignmentExpression;

 public:
    explicit TableDefaultAction(ControlPlaneAssignmentSet actionAssignment);

    bool operator<(const ControlPlaneItem &other) const override;

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(TableDefaultAction);
};

/**************************************************************************************************
WildCardMatchEntry
**************************************************************************************************/

/// A wildcard table match entry can be used to match all possible actions and does not impose
/// constraints on key values.
class WildCardMatchEntry : public TableMatchEntry {
 public:
    explicit WildCardMatchEntry(ControlPlaneAssignmentSet actionAssignment, int32_t priority);

    bool operator<(const ControlPlaneItem &other) const override;

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(WildCardMatchEntry);
};

/**************************************************************************************************
TableConfiguration
**************************************************************************************************/

/// The active set of table entries. Sorted by type.
using TableEntrySet =
    std::set<std::reference_wrapper<const TableMatchEntry>, std::less<const TableMatchEntry>>;

/// Concrete configuration of a control plane table. May contain arbitrary many table match entries.
class TableConfiguration : public ControlPlaneItem {
    /// The control plane name of the table that is being configured.
    cstring _tableName;

    /// The default behavior of the table when it is not configured.
    TableDefaultAction _defaultTableAction;

    /// The set of table entries in the configuration.
    TableEntrySet _tableEntries;

    /// Second-order sorting function for table entries. Sorts entries by priority.
    class CompareTableMatch {
     public:
        bool operator()(const TableMatchEntry &left, const TableMatchEntry &right);
    };

 public:
    explicit TableConfiguration(cstring tableName, TableDefaultAction defaultTableAction,
                                TableEntrySet tableEntries);

    bool operator<(const ControlPlaneItem &other) const override;

    /// Adds a new table entry.
    int addTableEntry(const TableMatchEntry &tableMatchEntry, bool replace);

    /// Delete an existing table entry.
    size_t deleteTableEntry(const TableMatchEntry &tableMatchEntry);

    /// Clear all table entries.
    void clearTableEntries();

    /// Set the default action for this table.
    void setDefaultTableAction(TableDefaultAction defaultTableAction);

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(TableConfiguration);
};

/**************************************************************************************************
ParserValueSet
**************************************************************************************************/

/// Implements a parser value set as specified in
/// https://p4.org/p4-spec/docs/P4-16-working-spec.html#sec-value-set.
/// TODO: Actually implement all the elments in the value set.
class ParserValueSet : public ControlPlaneItem {
    cstring _name;

 public:
    explicit ParserValueSet(cstring name);

    bool operator<(const ControlPlaneItem &other) const override;

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(ParserValueSet);
};

/**************************************************************************************************
ActionProfile
**************************************************************************************************/

/// An action profile. Action profiles are programmed like a table, but each table associated
/// with the respective table shares the action profile configuration. Hence, we use a set of table
/// control plane names to represent this data structure.
class ActionProfile : public ControlPlaneItem {
    /// The control plane name of the action profile.
    cstring _name;

    /// The control plane names of the tables associated with this action profile.
    std::set<cstring> _associatedTables;

 public:
    explicit ActionProfile(cstring name) : _name(name){};
    explicit ActionProfile(cstring name, std::set<cstring> associatedTables)
        : _name(name), _associatedTables(std::move(associatedTables)) {}

    bool operator<(const ControlPlaneItem &other) const override;

    /// @returns the control plane name of the action profile.
    [[nodiscard]] cstring name() const { return _name; }

    /// Get the set of control plane names of the tables associated with this action profile.
    [[nodiscard]] const std::set<cstring> &associatedTables() const;

    /// Add the control plane name of a  table to the set of associated tables.
    void addAssociatedTable(cstring table);

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(ActionProfile);
};

/**************************************************************************************************
ActionSelector
**************************************************************************************************/

/// An action selector. Action selectors are programmed like a table, but each table associated
/// with the respective table shares the action selector configuration. Hence, we use a set of table
/// control plane names to represent this data structure.
class ActionSelector : public ControlPlaneItem {
    /// The reference to the action profile associated with the selector.
    std::reference_wrapper<ActionProfile> _actionProfile;

    /// The control plane names of the tables associated with this action profile.
    std::set<cstring> _associatedTables;

 public:
    explicit ActionSelector(ActionProfile &actionProfile) : _actionProfile(actionProfile){};
    explicit ActionSelector(ActionProfile &actionProfile, std::set<cstring> associatedTables)
        : _actionProfile(actionProfile), _associatedTables(std::move(associatedTables)) {}

    bool operator<(const ControlPlaneItem &other) const override;

    /// Get the reference to the action profile associated with the selector.
    [[nodiscard]] const ActionProfile &actionProfile() const;

    /// Get the set of control plane names of the tables associated with this action profile.
    [[nodiscard]] const std::set<cstring> &associatedTables() const;

    /// Add the control plane name of a  table to the set of associated tables.
    void addAssociatedTable(cstring table);

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(ActionSelector);
};

/**************************************************************************************************
TableActionSelectorConfiguration
**************************************************************************************************/
class TableActionSelectorConfiguration : public TableConfiguration {
 public:
    explicit TableActionSelectorConfiguration(cstring tableName,
                                              TableDefaultAction defaultTableAction,
                                              TableEntrySet tableEntries);

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(TableActionSelectorConfiguration);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_CONTROL_PLANE_OBJECTS_H_ */
