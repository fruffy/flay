#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_

#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <utility>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/solver.h"
#include "lib/castable.h"

namespace P4Tools::Flay {

class ControlPlaneItem : public ICastable {
 public:
    ControlPlaneItem() = default;
    ControlPlaneItem(const ControlPlaneItem &) = default;
    ControlPlaneItem(ControlPlaneItem &&) = default;
    ControlPlaneItem &operator=(const ControlPlaneItem &) = default;
    ControlPlaneItem &operator=(ControlPlaneItem &&) = default;
    ~ControlPlaneItem() override = default;

    virtual bool operator<(const ControlPlaneItem &other) const = 0;

    [[nodiscard]] virtual const IR::Expression *computeControlPlaneConstraint() const = 0;
};

/// TODO: Make this an unordered set.
using TableKeySet = std::set<std::pair<const IR::SymbolicVariable &, const IR::Literal &>>;

class TableMatchEntry : public ControlPlaneItem {
    TableKeySet matches;

    const Constraint *actionAssignment;

    int32_t priority;

 public:
    explicit TableMatchEntry(const Constraint *actionAssignment, int32_t priority)
        : actionAssignment(actionAssignment), priority(priority) {}

    ~TableMatchEntry() override = default;
    TableMatchEntry(const TableMatchEntry &) = default;
    TableMatchEntry(TableMatchEntry &&) = default;
    TableMatchEntry &operator=(const TableMatchEntry &) = default;
    TableMatchEntry &operator=(TableMatchEntry &&) = default;

    [[nodiscard]] const TableKeySet &getMatches() const { return matches; }

    [[nodiscard]] const Constraint *getActionAssignment() const { return actionAssignment; }

    [[nodiscard]] int32_t getPriority() const { return priority; }

    void addMatch(const IR::SymbolicVariable &symbolicVariable, const IR::Literal &literal) {
        matches.insert({symbolicVariable, literal});
    }

    void addTableKeySet(const TableKeySet &tableKeySet) {
        matches.insert(tableKeySet.begin(), tableKeySet.end());
    }

    bool operator<(const ControlPlaneItem &other) const override {
        if (static_cast<const ControlPlaneItem *>(this) == &other) {
            return false;
        }
        auto otherMatches = static_cast<const TableMatchEntry &>(other).matches;
        if (matches.size() != otherMatches.size()) {
            return matches.size() < otherMatches.size();
        }
        for (const auto &match : matches) {
            auto it = otherMatches.find(match);
            if (it == otherMatches.end()) {
                return true;
            }
            const auto &symbolicVariable = match.first;
            const auto &otherSymbolicVariable = it->first;
            if (symbolicVariable != otherSymbolicVariable) {
                return symbolicVariable < otherSymbolicVariable;
            }
            const auto &literal = match.second;
            const auto &otherLiteral = it->second;
            if (literal != otherLiteral) {
                return literal < otherLiteral;
            }
        }
        return false;
    }

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override {
        if (matches.size() == 0) {
            return actionAssignment;
        }
        const IR::Expression *matchExpression = actionAssignment;
        for (const auto &match : matches) {
            const auto &symbolicVariable = match.first;
            const auto &assignment = match.second;
            matchExpression =
                new IR::LAnd(matchExpression, new IR::Equ(&symbolicVariable, &assignment));
        }
        return matchExpression;
    }
};

class TableConfiguration : public ControlPlaneItem {
    cstring tableName;

    TableMatchEntry defaultConfig;

    using TableEntrySet =
        std::set<std::reference_wrapper<const TableMatchEntry>, std::less<const TableMatchEntry>>;
    TableEntrySet tableEntries;

    class CompareTableMatch {
     public:
        bool operator()(const TableMatchEntry &left, const TableMatchEntry &right) {
            return left.getPriority() < right.getPriority();
        }
    };

 public:
    explicit TableConfiguration(cstring tableName, TableMatchEntry defaultConfig,
                                TableEntrySet tableEntries)
        : tableName(tableName),
          defaultConfig(std::move(defaultConfig)),
          tableEntries(std::move(tableEntries)) {}

    ~TableConfiguration() override = default;
    TableConfiguration(const TableConfiguration &) = default;
    TableConfiguration(TableConfiguration &&) = default;
    TableConfiguration &operator=(const TableConfiguration &) = default;
    TableConfiguration &operator=(TableConfiguration &&) = default;

    bool operator<(const ControlPlaneItem &other) const override {
        if (static_cast<const ControlPlaneItem *>(this) == &other) {
            return false;
        }
        return tableName < static_cast<const TableConfiguration &>(other).tableName;
    }

    void addTableEntry(const TableMatchEntry &tableMatchEntry) {
        tableEntries.emplace(tableMatchEntry);
    }

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override {
        const IR::Expression *matchExpression = defaultConfig.computeControlPlaneConstraint();
        if (tableEntries.size() == 0) {
            return matchExpression;
        }
        std::priority_queue sortedTableEntries(tableEntries.begin(), tableEntries.end(),
                                               CompareTableMatch());
        while (!sortedTableEntries.empty()) {
            const auto &tableEntry = sortedTableEntries.top().get();
            matchExpression = new IR::Mux(tableEntry.computeControlPlaneConstraint(),
                                          tableEntry.getActionAssignment(), matchExpression);
            sortedTableEntries.pop();
        }

        return matchExpression;
    }
};

class CloneSession : public ControlPlaneItem {
    std::optional<uint32_t> sessionId;

 public:
    explicit CloneSession(std::optional<uint32_t> sessionId) : sessionId(sessionId) {}

    ~CloneSession() override = default;
    CloneSession(const CloneSession &) = default;
    CloneSession(CloneSession &&) = default;
    CloneSession &operator=(const CloneSession &) = default;
    CloneSession &operator=(CloneSession &&) = default;

    bool operator<(const ControlPlaneItem &other) const override {
        if (static_cast<const ControlPlaneItem *>(this) == &other) {
            return false;
        }
        return sessionId < static_cast<const CloneSession &>(other).sessionId;
    }

    void setSessionId(uint32_t sessionId) { this->sessionId = sessionId; }

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override {
        const auto *cloneActive =
            ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), "clone_session_active");
        if (!sessionId.has_value()) {
            return new IR::Equ(cloneActive, new IR::BoolLiteral(false));
        }
        const auto *sessionIdExpr = IR::getConstant(IR::getBitType(32), sessionId.value());
        return new IR::LAnd(new IR::Equ(cloneActive, new IR::BoolLiteral(false)), sessionIdExpr);
    }
};

/// The constraints imposed by the control plane on the program.
using ControlPlaneConstraints =
    std::map<cstring, std::reference_wrapper<ControlPlaneItem>, std::less<>>;

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_ */
