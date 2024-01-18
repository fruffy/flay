#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"

#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "ir/ir-generated.h"

namespace P4Tools::Flay {

const IR::SymbolicVariable *ControlPlaneState::getTableActive(cstring tableName) {
    cstring label = tableName + "_configured";
    return ToolsVariables::getSymbolicVariable(IR::Type_Boolean::get(), label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableKey(cstring tableName, cstring fieldName,
                                                           const IR::Type *type) {
    cstring label = tableName + "_key_" + fieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableTernaryMask(cstring tableName,
                                                                   cstring fieldName,
                                                                   const IR::Type *type) {
    cstring label = tableName + "_mask_" + fieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableMatchLpmPrefix(cstring tableName,
                                                                      cstring fieldName,
                                                                      const IR::Type *type) {
    cstring label = tableName + "_lpm_prefix_" + fieldName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableActionArg(cstring tableName,
                                                                 cstring actionName,
                                                                 cstring parameterName,
                                                                 const IR::Type *type) {
    cstring label = tableName + "_" + actionName + "_param_" + parameterName;
    return ToolsVariables::getSymbolicVariable(type, label);
}

const IR::SymbolicVariable *ControlPlaneState::getTableActionChoice(cstring tableName) {
    cstring label = tableName + "_action";
    return ToolsVariables::getSymbolicVariable(IR::Type_String::get(), label);
}

ControlPlaneConstraints ControlPlaneStateInitializer::getDefaultConstraints() const {
    return defaultConstraints;
}

// std::optional<TableKeySet> computeEntryKeySet(const IR::P4Table &table, const IR::Entry &entry,
//                                               const IR::Key &key) {
//     TableKeySet keySet;
//     auto numKeys = key.keyElements.size();
//     BUG_CHECK(numKeys == entry.keys->size(),
//               "The entry key list and key match list must be equal in size.");
//     for (size_t idx = 0; idx < numKeys; ++idx) {
//         const auto *keyElement = key.keyElements.at(idx);
//         const auto *keyExpr = keyElement->expression;
//         BUG_CHECK(keyExpr != nullptr, "Entry %1% in table %2% is null", entry, table);
//         const auto *entryKey = entry.keys->components.at(idx);
//         // DefaultExpressions always match, so do not even consider them in the equation.
//         if (entryKey->is<IR::DefaultExpression>()) {
//             continue;
//         }
//         if (const auto *rangeExpr = entryKey->to<IR::Range>()) {
//             const auto *minKey = rangeExpr->left;
//             const auto *maxKey = rangeExpr->right;
//         } else if (const auto *maskExpr = entryKey->to<IR::Mask>()) {
//             entryMatchCondition = new IR::LAnd(
//                 entryMatchCondition, new IR::Equ(new IR::BAnd(keyExpr, maskExpr->right),
//                                                  new IR::BAnd(maskExpr->left, maskExpr->right)));
//         } else {
//             entryMatchCondition = new IR::LAnd(entryMatchCondition, new IR::Equ(keyExpr,
//             entryKey));
//         }
//     }
//     return keySet;
// }

bool ControlPlaneStateInitializer::preorder(const IR::P4Table *table) {
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(*table, properties);
    if (properties.tableIsImmutable) {
        return false;
    }
    auto tableName = table->controlPlaneName();
    const auto *actionName = TableUtils::getDefaultActionName(*table);
    auto defaultEntry = TableMatchEntry(
        new IR::Equ(ControlPlaneState::getTableActionChoice(tableName),
                    new IR::StringLiteral(IR::Type_String::get(), actionName->path->name)),
        0, {});
    TableEntrySet initialTableEntries;
    const auto *entries = table->getEntries();
    if (entries != nullptr) {
        ::error("Initial entries in a table are not supported yet.");
        return false;
        // for (const auto &entry : entries->entries) {
        //     const auto *action = entry->getAction();
        //     const auto *actionCall = action->checkedTo<IR::MethodCallExpression>();
        //     const auto *methodName = actionCall->method->checkedTo<IR::PathExpression>();

        //     auto *actionAssignment =
        //         new IR::Equ(ControlPlaneState::getTableActionChoice(tableName),
        //                     new IR::StringLiteral(IR::Type_String::get(),
        //                     methodName->path->name));
        //     const auto *tableMatchEntry = new TableMatchEntry(actionAssignment, 100, {});
        //     initialTableEntries.insert(*tableMatchEntry);
        // }
    }

    defaultConstraints.insert(
        {tableName, *new TableConfiguration(tableName, defaultEntry, initialTableEntries)});
    return false;
}

}  // namespace P4Tools::Flay
