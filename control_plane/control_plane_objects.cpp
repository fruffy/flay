#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"

#include <cstdlib>
#include <utility>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/substitute_placeholders.h"
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
TableMatchKeys
**************************************************************************************************/

TableMatchKey::TableMatchKey(cstring name) : _name(name){};

cstring TableMatchKey::name() const { return _name; }

bool TableMatchKey::operator<(const ControlPlaneItem &other) const {
    return typeid(*this) == typeid(other) ? _name < static_cast<const TableMatchKey &>(other)._name
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

ControlPlaneAssignmentSet TableMatchKey::computeControlPlaneAssignments() const {
    P4C_UNIMPLEMENTED("computeControlPlaneAssignments");
}

ExactTableMatchKey::ExactTableMatchKey(cstring name, const IR::SymbolicVariable *variable,
                                       const IR::Expression *value)
    : TableMatchKey(name), _variable(variable), _keyExpression(value) {}

const IR::SymbolicVariable *ExactTableMatchKey::variable() const { return _variable; }

const IR::Expression *ExactTableMatchKey::keyExpression() const { return _keyExpression; }

const IR::Expression *ExactTableMatchKey::computeControlPlaneConstraint() const {
    return new IR::Equ(_variable, _keyExpression);
}

TernaryTableMatchKey::TernaryTableMatchKey(cstring name, const IR::SymbolicVariable *variable,
                                           const IR::SymbolicVariable *mask,
                                           const IR::Expression *_keyExpression)
    : TableMatchKey(name), _variable(variable), _mask(mask), _keyExpression(_keyExpression) {}

const IR::SymbolicVariable *TernaryTableMatchKey::variable() const { return _variable; }

const IR::Expression *TernaryTableMatchKey::keyExpression() const { return _keyExpression; }

const IR::SymbolicVariable *TernaryTableMatchKey::mask() const { return _mask; }

const IR::Expression *TernaryTableMatchKey::computeControlPlaneConstraint() const {
    return new IR::Equ(new IR::BAnd(_keyExpression, _mask), new IR::BAnd(_variable, _mask));
}

LpmTableMatchKey::LpmTableMatchKey(cstring name, const IR::SymbolicVariable *variable,
                                   const IR::Expression *value, const IR::SymbolicVariable *prefix)
    : TableMatchKey(name), _variable(variable), _keyExpression(value), _prefixVar(prefix) {}

const IR::SymbolicVariable *LpmTableMatchKey::variable() const { return _variable; }

const IR::Expression *LpmTableMatchKey::keyExpression() const { return _keyExpression; }

const IR::SymbolicVariable *LpmTableMatchKey::prefix() const { return _prefixVar; }

const IR::Expression *LpmTableMatchKey::computeControlPlaneConstraint() const {
    // The maxReturn is the maximum vale for the given bit width. This value is
    // shifted by the mask variable to create a mask (and with that, a prefix).
    const auto *keyType = _variable->type;
    auto keyWidth = keyType->width_bits();
    auto maxReturn = IR::getMaxBvVal(keyWidth);
    auto *prefix = new IR::Sub(IR::Constant::get(keyType, keyWidth), _prefixVar);
    const IR::Expression *lpmMask = new IR::Shl(IR::Constant::get(keyType, maxReturn), prefix);
    return new IR::LAnd(
        // This is the actual LPM match under the shifted mask (the prefix).
        new IR::Leq(_prefixVar, IR::Constant::get(keyType, keyWidth)),
        // The mask variable shift should not be larger than the key width.
        new IR::Equ(new IR::BAnd(_keyExpression, lpmMask), new IR::BAnd(_variable, lpmMask)));
}

OptionalMatchKey::OptionalMatchKey(cstring name, const IR::SymbolicVariable *variable,
                                   const IR::Expression *value)
    : TableMatchKey(name), _variable(variable), _keyExpression(value) {}

const IR::SymbolicVariable *OptionalMatchKey::variable() const { return _variable; }

const IR::Expression *OptionalMatchKey::keyExpression() const { return _keyExpression; }

const IR::Expression *OptionalMatchKey::computeControlPlaneConstraint() const {
    return new IR::Equ(_variable, _keyExpression);
}

SelectorMatchKey::SelectorMatchKey(cstring name, const IR::SymbolicVariable *variable,
                                   const IR::Expression *value)
    : TableMatchKey(name), _variable(variable), _keyExpression(value) {}

const IR::SymbolicVariable *SelectorMatchKey::variable() const { return _variable; }

const IR::Expression *SelectorMatchKey::keyExpression() const { return _keyExpression; }

const IR::Expression *SelectorMatchKey::computeControlPlaneConstraint() const {
    return new IR::Equ(_variable, _keyExpression);
}

RangeTableMatchKey::RangeTableMatchKey(cstring name, const IR::SymbolicVariable *minKey,

                                       const IR::SymbolicVariable *maxKey,
                                       const IR::Expression *value)

    : TableMatchKey(name), _minKey(minKey), _maxKey(maxKey), _keyExpression(value) {}

const IR::SymbolicVariable *RangeTableMatchKey::minKey() const { return _minKey; }

const IR::Expression *RangeTableMatchKey::maxKey() const { return _maxKey; }

const IR::Expression *RangeTableMatchKey::keyExpression() const { return _keyExpression; }

const IR::Expression *RangeTableMatchKey::computeControlPlaneConstraint() const {
    return new IR::LAnd(
        new IR::LAnd(new IR::Lss(_minKey, _maxKey), new IR::Leq(_minKey, _keyExpression)),
        new IR::Leq(_keyExpression, _maxKey));
}

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

void TableConfiguration::setTableKeyMatch(const IR::Expression *tableKeyMatch) {
    _tableKeyMatch = tableKeyMatch;
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

ControlPlaneAssignmentSet TableConfiguration::computeControlPlaneAssignments() const {
    auto defaultAssignments = _defaultTableAction.computeControlPlaneAssignments();
    defaultAssignments.emplace(*ControlPlaneState::getTableActive(_tableName),
                               *IR::BoolLiteral::get(_tableEntries.size() > 0));
    if (_tableEntries.size() == 0) {
        return defaultAssignments;
    }

    for (const auto &tableEntry : _tableEntries) {
        const auto &actionAssignments = tableEntry.get().actionAssignment();
        const auto keyAssignments = tableEntry.get().computeControlPlaneAssignments();
        const auto *constraint = _tableKeyMatch->apply(SubstituteSymbolicVariable(keyAssignments));

        for (const auto &[variable, assignment] : actionAssignments) {
            auto it = defaultAssignments.find(variable);
            if (it != defaultAssignments.end()) {
                it->second = *new IR::Mux(constraint, &assignment.get(), &it->second.get());
            } else {
                defaultAssignments.insert({variable, assignment});
            }
        }
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

ControlPlaneAssignmentSet ActionProfile::computeControlPlaneAssignments() const {
    // Action profiles are indirect and associated with the constraints of a table.
    return {};
}

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

ControlPlaneAssignmentSet ActionSelector::computeControlPlaneAssignments() const {
    // Action profiles are indirect and associated with the constraints of a table.

    return {};
}

/**************************************************************************************************
TableActionSelectorConfiguration
**************************************************************************************************/

TableActionSelectorConfiguration::TableActionSelectorConfiguration(
    cstring tableName, TableDefaultAction defaultTableAction, TableEntrySet tableEntries)
    : TableConfiguration(tableName, std::move(defaultTableAction), std::move(tableEntries)) {}

ControlPlaneAssignmentSet TableActionSelectorConfiguration::computeControlPlaneAssignments() const {
    // This does nothing currently.
    return {};
}

}  // namespace P4Tools::Flay
