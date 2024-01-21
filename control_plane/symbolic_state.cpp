#include "backends/p4tools/modules/flay/control_plane/symbolic_state.h"

#include "backends/p4tools/common/lib/constants.h"
#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "ir/irutils.h"
#include "lib/error.h"

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

std::optional<TableKeySet> computeEntryKeySet(const IR::P4Table &table, const IR::Entry &entry) {
    ASSIGN_OR_RETURN_WITH_MESSAGE(const auto &key, table.getKey(), std::nullopt,
                                  ::error("Table %1% has no key.", table));
    auto numKeys = key.keyElements.size();
    RETURN_IF_FALSE_WITH_MESSAGE(
        numKeys == entry.keys->size(), std::nullopt,
        ::error("Entry key list and key match list must be equal in size."));
    TableKeySet keySet;
    auto tableName = table.controlPlaneName();
    for (size_t idx = 0; idx < numKeys; ++idx) {
        const auto *keyElement = key.keyElements.at(idx);
        const auto *keyExpr = keyElement->expression;
        RETURN_IF_FALSE_WITH_MESSAGE(keyExpr != nullptr, std::nullopt,
                                     ::error("Entry %1% in table %2% is null"));
        ASSIGN_OR_RETURN_WITH_MESSAGE(
            const auto &nameAnnotation, keyElement->getAnnotation("name"), std::nullopt,
            ::error("Key %1% in table %2% does not have a name annotation.", keyElement, table));
        auto fieldName = nameAnnotation.getName();
        const auto matchType = keyElement->matchType->toString();
        const auto *keyType = keyExpr->type;
        const auto *keySymbol = ControlPlaneState::getTableKey(tableName, fieldName, keyType);
        const auto *entryKey = entry.keys->components.at(idx);
        if (matchType == P4Constants::MATCH_KIND_EXACT) {
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &exactValue, entryKey->to<IR::Literal>(), std::nullopt,
                ::error("Entry %1% in table %2% is not a literal.", entryKey, table));
            keySet.emplace(*keySymbol, exactValue);
        } else if (matchType == P4Constants::MATCH_KIND_LPM) {
            const auto *lpmPrefixSymbol =
                ControlPlaneState::getTableMatchLpmPrefix(tableName, fieldName, keyType);
            if (entryKey->is<IR::DefaultExpression>()) {
                keySet.emplace(*keySymbol, *IR::getConstant(keyType, 0));
                keySet.emplace(*lpmPrefixSymbol, *IR::getConstant(keyType, 0));
            } else if (const auto *maskExpr = entryKey->to<IR::Mask>()) {
                ASSIGN_OR_RETURN_WITH_MESSAGE(
                    const auto &maskLeft, maskExpr->left->to<IR::Literal>(), std::nullopt,
                    ::error("Left mask element %1% is not a literal.", maskExpr->left));
                ASSIGN_OR_RETURN_WITH_MESSAGE(
                    const auto &maskRight, maskExpr->right->to<IR::Literal>(), std::nullopt,
                    ::error("Right mask element %1% is not a literal.", maskExpr->right));
                keySet.emplace(*keySymbol, maskLeft);
                keySet.emplace(*lpmPrefixSymbol, maskRight);
            } else {
                ASSIGN_OR_RETURN_WITH_MESSAGE(
                    const auto &exactValue, entryKey->to<IR::Literal>(), std::nullopt,
                    ::error("Entry %1% in table %2% is not a literal.", entryKey, table));
                keySet.emplace(*keySymbol, exactValue);
                keySet.emplace(*lpmPrefixSymbol, *IR::getConstant(keyType, keyType->width_bits()));
            }
        } else if (matchType == P4Constants::MATCH_KIND_TERNARY) {
            const auto *maskSymbol =
                ControlPlaneState::getTableTernaryMask(tableName, fieldName, keyType);
            if (entryKey->is<IR::DefaultExpression>()) {
                keySet.emplace(*keySymbol, *IR::getConstant(keyType, 0));
                keySet.emplace(*maskSymbol, *IR::getConstant(keyType, 0));
            } else if (const auto *maskExpr = entryKey->to<IR::Mask>()) {
                ASSIGN_OR_RETURN_WITH_MESSAGE(
                    const auto &maskLeft, maskExpr->left->to<IR::Literal>(), std::nullopt,
                    ::error("Left mask element %1% is not a literal.", maskExpr->left));
                ASSIGN_OR_RETURN_WITH_MESSAGE(
                    const auto &maskRight, maskExpr->right->to<IR::Literal>(), std::nullopt,
                    ::error("Right mask element %1% is not a literal.", maskExpr->right));
                keySet.emplace(*keySymbol, maskLeft);
                keySet.emplace(*maskSymbol, maskRight);
            } else {
                ASSIGN_OR_RETURN_WITH_MESSAGE(
                    const auto &exactValue, entryKey->to<IR::Literal>(), std::nullopt,
                    ::error("Entry %1% in table %2% is not a literal.", entryKey, table));
                keySet.emplace(*keySymbol, exactValue);
                keySet.emplace(*maskSymbol, *IR::getMaxValueConstant(keyType));
            }
        } else {
            ::error("Match type of key %1% is not supported.", keyElement);
            return std::nullopt;
        }
    }
    return keySet;
}

ControlPlaneStateInitializer::ControlPlaneStateInitializer(const P4::ReferenceMap &refMap)
    : refMap_(refMap) {}

bool ControlPlaneStateInitializer::preorder(const IR::P4Table *table) {
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(*table, properties);
    RETURN_IF_FALSE(!properties.tableIsImmutable, false);

    auto tableName = table->controlPlaneName();
    const auto *actionName = TableUtils::getDefaultActionName(*table);
    auto defaultEntry = TableMatchEntry(
        new IR::Equ(ControlPlaneState::getTableActionChoice(tableName),
                    new IR::StringLiteral(IR::Type_String::get(), actionName->path->name)),
        0, {});
    TableEntrySet initialTableEntries;
    const auto *entries = table->getEntries();
    if (entries != nullptr) {
        for (const auto &entry : entries->entries) {
            const auto *action = entry->getAction();
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &actionCall, action->to<IR::MethodCallExpression>(), false,
                ::error("Action %1% in table %2% is not a method call.", action, table));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &methodName, actionCall.method->to<IR::PathExpression>(), false,
                ::error("Action %1% in table %2% is not a path expression.", action, table));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                auto &actionDecl, refMap_.get().getDeclaration(methodName.path, false), false,
                ::error("Action reference %1% not found in the reference map", methodName));
            auto *actionAssignment = new IR::Equ(
                ControlPlaneState::getTableActionChoice(tableName),
                new IR::StringLiteral(IR::Type_String::get(), actionDecl.controlPlaneName()));
            ASSIGN_OR_RETURN(auto entryKeySet, computeEntryKeySet(*table, *entry), false);
            initialTableEntries.insert(*new TableMatchEntry(actionAssignment, 100, entryKeySet));
        }
    }

    defaultConstraints.insert(
        {tableName, *new TableConfiguration(tableName, defaultEntry, initialTableEntries)});
    return false;
}

}  // namespace P4Tools::Flay
