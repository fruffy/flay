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

}  // namespace P4Tools::ControlPlaneState

namespace P4Tools::Flay {

TableMatchEntry::TableMatchEntry(const Constraint *actionAssignment, int32_t priority,
                                 const TableKeySet &matches)
    : actionAssignment(actionAssignment),
      priority(priority),
      matchExpression(computeMatchExpression(matches)) {}

const IR::Expression *TableMatchEntry::computeMatchExpression(const TableKeySet &matches) {
    if (matches.size() == 0) {
        return IR::getBoolLiteral(false);
    }
    const IR::Expression *matchExpression = nullptr;
    // Precompute the match expression in the constructor.
    for (const auto &match : matches) {
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

int32_t TableMatchEntry::getPriority() const { return priority; }

const IR::Expression *TableMatchEntry::getActionAssignment() const { return actionAssignment; }

bool TableMatchEntry::operator<(const ControlPlaneItem &other) const {
    // Table match entries are only compared based on the match expression.
    return typeid(*this) == typeid(other)
               ? matchExpression->isSemanticallyLess(
                     *(dynamic_cast<const TableMatchEntry &>(other)).matchExpression)
               : typeid(*this).hash_code() < typeid(other).hash_code();
}

const IR::Expression *TableMatchEntry::computeControlPlaneConstraint() const {
    return matchExpression;
}

bool TableConfiguration::CompareTableMatch::operator()(const TableMatchEntry &left,
                                                       const TableMatchEntry &right) {
    return left.getPriority() > right.getPriority();
}

TableConfiguration::TableConfiguration(cstring tableName, TableDefaultAction defaultTableAction,
                                       TableEntrySet tableEntries)
    : tableName_(tableName),
      defaultTableAction_(std::move(defaultTableAction)),
      tableEntries_(std::move(tableEntries)) {}

bool TableConfiguration::operator<(const ControlPlaneItem &other) const {
    return typeid(*this) == typeid(other)
               ? tableName_ < static_cast<const TableConfiguration &>(other).tableName_
               : typeid(*this).hash_code() < typeid(other).hash_code();
}

int TableConfiguration::addTableEntry(const TableMatchEntry &tableMatchEntry, bool replace) {
    if (replace) {
        tableEntries_.erase(tableMatchEntry);
    }
    return tableEntries_.emplace(tableMatchEntry).second ? EXIT_SUCCESS : EXIT_FAILURE;
}

size_t TableConfiguration::deleteTableEntry(const TableMatchEntry &tableMatchEntry) {
    return tableEntries_.erase(tableMatchEntry);
}

void TableConfiguration::setDefaultTableAction(TableDefaultAction defaultTableAction) {
    defaultTableAction_ = std::move(defaultTableAction);
}

const IR::Expression *TableConfiguration::computeControlPlaneConstraint() const {
    const auto *tableConfigured = new IR::Equ(ControlPlaneState::getTableActive(tableName_),
                                              IR::getBoolLiteral(tableEntries_.size() > 0));
    const IR::Expression *matchExpression = defaultTableAction_.computeControlPlaneConstraint();
    if (tableEntries_.size() == 0) {
        return new IR::LAnd(matchExpression, tableConfigured);
    }
    std::priority_queue sortedTableEntries(tableEntries_.begin(), tableEntries_.end(),
                                           CompareTableMatch());
    while (!sortedTableEntries.empty()) {
        const auto &tableEntry = sortedTableEntries.top().get();
        matchExpression = new IR::Mux(tableEntry.computeControlPlaneConstraint(),
                                      tableEntry.getActionAssignment(), matchExpression);
        sortedTableEntries.pop();
    }

    return new IR::LAnd(matchExpression, tableConfigured);
}

ParserValueSet::ParserValueSet(cstring name) : name_(name) {}

bool ParserValueSet::operator<(const ControlPlaneItem &other) const {
    return typeid(*this) == typeid(other) ? name_ < static_cast<const ParserValueSet &>(other).name_
                                          : typeid(*this).hash_code() < typeid(other).hash_code();
}

const IR::Expression *ParserValueSet::computeControlPlaneConstraint() const {
    return new IR::Equ(ControlPlaneState::getParserValueSetConfigured(name_),
                       IR::getBoolLiteral(false));
}

}  // namespace P4Tools::Flay
