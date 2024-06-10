#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"

#include <cstdlib>
#include <queue>
#include <utility>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/irutils.h"

namespace P4Tools::ControlPlaneState {

const IR::SymbolicVariable *getParserValueSetConfigured(cstring parserValueSetName) {
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(),
                                               "pvs_configured_" + parserValueSetName);
}

const IR::SymbolicVariable *getDefaultActionVariable(cstring tableName) {
    return new IR::SymbolicVariable(IR::Type_String::get(), tableName + "_default_action");
}

}  // namespace P4Tools::ControlPlaneState

namespace P4Tools::Flay {

/**************************************************************************************************
TableMatchEntry
**************************************************************************************************/

TableMatchEntry::TableMatchEntry(ControlPlaneAssignmentSet actionAssignment, int32_t priority,
                                 const ControlPlaneAssignmentSet &matches)
    : _actionAssignment(std::move(actionAssignment)),
      _priority(priority),
      _matchExpression(computeConstraintExpression(matches)),
      _actionAssignmentExpression(computeConstraintExpression(_actionAssignment)),
      _matches(matches) {}

int32_t TableMatchEntry::priority() const { return _priority; }

ControlPlaneAssignmentSet TableMatchEntry::actionAssignment() const { return _actionAssignment; }

const IR::Expression *TableMatchEntry::actionAssignmentExpression() const {
    return _actionAssignmentExpression;
}

bool TableMatchEntry::operator<(const ControlPlaneItem &other) const {
    // Table match entries are only compared based on the match expression.
    return typeid(*this) == typeid(other)
               ? _matchExpression->isSemanticallyLess(
                     *(dynamic_cast<const TableMatchEntry &>(other))._matchExpression)
               : typeid(*this).hash_code() < typeid(other).hash_code();
}

const IR::Expression *TableMatchEntry::computeControlPlaneConstraint() const {
    return _matchExpression;
}

ControlPlaneAssignmentSet TableMatchEntry::computeControlPlaneAssignments() const {
    return _matches;
}

/**************************************************************************************************
TableDefaultAction
**************************************************************************************************/

TableDefaultAction::TableDefaultAction(ControlPlaneAssignmentSet actionAssignment)
    : _actionAssignment(std::move(actionAssignment)),
      _actionAssignmentExpression(computeConstraintExpression(_actionAssignment)) {}

bool TableDefaultAction::operator<(const ControlPlaneItem &other) const {
    // Table match entries are only compared based on the match expression.
    return typeid(*this) == typeid(other)
               ? _actionAssignmentExpression->isSemanticallyLess(
                     *(dynamic_cast<const TableDefaultAction &>(other))._actionAssignmentExpression)
               : typeid(*this).hash_code() < typeid(other).hash_code();
}

[[nodiscard]] const IR::Expression *TableDefaultAction::computeControlPlaneConstraint() const {
    return _actionAssignmentExpression;
}

ControlPlaneAssignmentSet TableDefaultAction::computeControlPlaneAssignments() const {
    return _actionAssignment;
}

/**************************************************************************************************
WildCardMatchEntry
**************************************************************************************************/

WildCardMatchEntry::WildCardMatchEntry(ControlPlaneAssignmentSet actionAssignment, int32_t priority)
    : TableMatchEntry(std::move(actionAssignment), priority, {}) {
    _matchExpression = IR::BoolLiteral::get(true);
}

bool WildCardMatchEntry::operator<(const ControlPlaneItem &other) const {
    // Table match entries are only compared based on the match expression.
    return typeid(*this) == typeid(other)
               ? _matchExpression->isSemanticallyLess(
                     *(dynamic_cast<const WildCardMatchEntry &>(other))._matchExpression)
               : typeid(*this).hash_code() < typeid(other).hash_code();
}

const IR::Expression *WildCardMatchEntry::computeControlPlaneConstraint() const {
    return _matchExpression;
}

ControlPlaneAssignmentSet WildCardMatchEntry::computeControlPlaneAssignments() const { return {}; }

/**************************************************************************************************
TableConfiguration
**************************************************************************************************/

bool TableConfiguration::CompareTableMatch::operator()(const TableMatchEntry &left,
                                                       const TableMatchEntry &right) {
    return left.priority() > right.priority();
}

TableConfiguration::TableConfiguration(cstring tableName, TableDefaultAction defaultTableAction,
                                       TableEntrySet tableEntries)
    : _tableName(tableName),
      _defaultTableAction(std::move(defaultTableAction)),
      _tableEntries(std::move(tableEntries)) {}

bool TableConfiguration::operator<(const ControlPlaneItem &other) const {
    return typeid(*this) == typeid(other)
               ? _tableName < static_cast<const TableConfiguration &>(other)._tableName
               : typeid(*this).hash_code() < typeid(other).hash_code();
}

int TableConfiguration::addTableEntry(const TableMatchEntry &tableMatchEntry, bool replace) {
    if (replace) {
        _tableEntries.erase(tableMatchEntry);
    }
    return _tableEntries.emplace(tableMatchEntry).second ? EXIT_SUCCESS : EXIT_FAILURE;
}

size_t TableConfiguration::deleteTableEntry(const TableMatchEntry &tableMatchEntry) {
    return _tableEntries.erase(tableMatchEntry);
}

void TableConfiguration::clearTableEntries() { _tableEntries.clear(); }

void TableConfiguration::setDefaultTableAction(TableDefaultAction defaultTableAction) {
    _defaultTableAction = std::move(defaultTableAction);
}

const IR::Expression *TableConfiguration::computeControlPlaneConstraint() const {
    const auto *tableConfigured = new IR::Equ(ControlPlaneState::getTableActive(_tableName),
                                              IR::BoolLiteral::get(_tableEntries.size() > 0));
    const IR::Expression *matchExpression = _defaultTableAction.computeControlPlaneConstraint();
    if (_tableEntries.size() == 0) {
        return new IR::LAnd(matchExpression, tableConfigured);
    }
    // TODO: Do we need this priority calculation?
    // Maybe we should consider resolving overlapping entries beforehand.
    std::priority_queue sortedTableEntries(_tableEntries.begin(), _tableEntries.end(),
                                           CompareTableMatch());
    while (!sortedTableEntries.empty()) {
        const auto &tableEntry = sortedTableEntries.top().get();
        matchExpression = new IR::Mux(tableEntry.computeControlPlaneConstraint(),
                                      tableEntry.actionAssignmentExpression(), matchExpression);
        sortedTableEntries.pop();
    }

    return new IR::LAnd(matchExpression, tableConfigured);
}

ControlPlaneAssignmentSet TableConfiguration::computeControlPlaneAssignments() const {
    auto defaultAssignments = _defaultTableAction.computeControlPlaneAssignments();
    defaultAssignments.emplace(*ControlPlaneState::getTableActive(_tableName),
                               *IR::BoolLiteral::get(true));
    if (_tableEntries.size() == 0) {
        return defaultAssignments;
    }
    for (const auto &tableEntry : _tableEntries) {
        const auto &matchAssignments = tableEntry.get().computeControlPlaneAssignments();
        const auto &actionAssignments = tableEntry.get().actionAssignment();
        defaultAssignments.insert(matchAssignments.begin(), matchAssignments.end());
        defaultAssignments.insert(actionAssignments.begin(), actionAssignments.end());
    }
    return defaultAssignments;
}

/**************************************************************************************************
ParserValueSet
**************************************************************************************************/

ParserValueSet::ParserValueSet(cstring name) : _name(name) {}

bool ParserValueSet::operator<(const ControlPlaneItem &other) const {
    return typeid(*this) == typeid(other) ? _name < static_cast<const ParserValueSet &>(other)._name
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

const IR::Expression *ParserValueSet::computeControlPlaneConstraint() const {
    return new IR::Equ(ControlPlaneState::getParserValueSetConfigured(_name),
                       IR::BoolLiteral::get(false));
}

ControlPlaneAssignmentSet ParserValueSet::computeControlPlaneAssignments() const {
    return {{*ControlPlaneState::getParserValueSetConfigured(_name), *IR::BoolLiteral::get(false)}};
}

/**************************************************************************************************
ActionProfile
**************************************************************************************************/

bool ActionProfile::operator<(const ControlPlaneItem &other) const {
    // There is only one action profile active, we ignore the set of associated tables.
    return typeid(*this) == typeid(other) ? false
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

const std::set<cstring> &ActionProfile::associatedTables() const { return _associatedTables; }

void ActionProfile::addAssociatedTable(cstring table) { _associatedTables.insert(table); }

const IR::Expression *ActionProfile::computeControlPlaneConstraint() const {
    // Action profiles are indirect and associated with the constraints of a table.
    return IR::BoolLiteral::get(true);
}

ControlPlaneAssignmentSet ActionProfile::computeControlPlaneAssignments() const { return {}; }

/**************************************************************************************************
ActionSelector
**************************************************************************************************/

bool ActionSelector::operator<(const ControlPlaneItem &other) const {
    // There is only one action selection active.
    // We ignore the set of associated tables and referenced profile.
    return typeid(*this) == typeid(other) ? false
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

const std::set<cstring> &ActionSelector::associatedTables() const { return _associatedTables; }

void ActionSelector::addAssociatedTable(cstring table) {
    _associatedTables.insert(table);
    // Note that we also add the table to the associated profile.
    _actionProfile.get().addAssociatedTable(table);
}

const ActionProfile &ActionSelector::actionProfile() const { return _actionProfile; }

const IR::Expression *ActionSelector::computeControlPlaneConstraint() const {
    // Action profiles are indirect and associated with the constraints of a table.
    return IR::BoolLiteral::get(true);
}

ControlPlaneAssignmentSet ActionSelector::computeControlPlaneAssignments() const { return {}; }

/**************************************************************************************************
TableActionSelectorConfiguration
**************************************************************************************************/

TableActionSelectorConfiguration::TableActionSelectorConfiguration(
    cstring tableName, TableDefaultAction defaultTableAction, TableEntrySet tableEntries)
    : TableConfiguration(tableName, std::move(defaultTableAction), std::move(tableEntries)) {}

const IR::Expression *TableActionSelectorConfiguration::computeControlPlaneConstraint() const {
    // This does nothing currently.
    return IR::BoolLiteral::get(true);
}

ControlPlaneAssignmentSet TableActionSelectorConfiguration::computeControlPlaneAssignments() const {
    // This does nothing currently.
    return {};
}

}  // namespace P4Tools::Flay
