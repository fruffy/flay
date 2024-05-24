#include "backends/p4tools/modules/flay/targets/tofino/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/table_utils.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/control_plane_objects.h"
#include "backends/p4tools/modules/flay/control_plane/return_macros.h"
#include "backends/p4tools/modules/flay/targets/tofino/constants.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace P4Tools::Flay::Tofino {

bool TofinoControlPlaneInitializer::computeMatch(const IR::Expression &entryKey,
                                                 const IR::SymbolicVariable &keySymbol,
                                                 cstring tableName, cstring fieldName,
                                                 cstring matchType, TableKeySet &keySet) {
    if (matchType == TofinoBaseConstants::MATCH_KIND_RANGE) {
        cstring minName = tableName + "_range_min_" + fieldName;
        cstring maxName = tableName + "_range_max_" + fieldName;
        const auto *minKey = ToolsVariables::getSymbolicVariable(keySymbol.type, minName);
        const auto *maxKey = ToolsVariables::getSymbolicVariable(keySymbol.type, maxName);
        // TODO: What does default expression mean as a table entry?
        if (entryKey.is<IR::DefaultExpression>()) {
            keySet.emplace(*minKey, *IR::Constant::get(keySymbol.type, 0));
            keySet.emplace(*maxKey, *IR::getMaxValueConstant(keySymbol.type));
            return true;
        }

        if (const auto *rangeExpr = entryKey.to<IR::Range>()) {
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &minExpr, rangeExpr->left->to<IR::Literal>(), false,
                ::error("Left mask element %1% is not a literal.", rangeExpr->left));
            ASSIGN_OR_RETURN_WITH_MESSAGE(
                const auto &maxExpr, rangeExpr->right->to<IR::Literal>(), false,
                ::error("Right mask element %1% is not a literal.", rangeExpr->right));
            keySet.emplace(*minKey, minExpr);
            keySet.emplace(*maxKey, maxExpr);
            return true;
        }
        ASSIGN_OR_RETURN_WITH_MESSAGE(const auto &exactValue, entryKey.to<IR::Literal>(), false,
                                      ::error("Entry %1% is not a literal.", entryKey));
        keySet.emplace(*minKey, exactValue);
        keySet.emplace(*maxKey, exactValue);
        return true;
    }
    if (matchType == TofinoBaseConstants::MATCH_KIND_OPT) {
        // TODO: What does default expression mean as a table entry?
        if (entryKey.is<IR::DefaultExpression>()) {
            return true;
        }
        ASSIGN_OR_RETURN_WITH_MESSAGE(const auto &exactValue, entryKey.to<IR::Literal>(), false,
                                      ::error("Entry %1% is not a literal.", entryKey));
        keySet.emplace(keySymbol, exactValue);
        return true;
    }

    return ControlPlaneStateInitializer::computeMatch(entryKey, keySymbol, tableName, fieldName,
                                                      matchType, keySet);
}

namespace {

std::optional<cstring> checkForActionProfile(const IR::P4Table &table,
                                             const P4::ReferenceMap &refMap) {
    const auto *impl = table.properties->getProperty("implementation");
    if (impl == nullptr) {
        return std::nullopt;
    }

    const auto *implExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const IR::IDeclaration *implDecl = nullptr;
    if (const auto *implPath = implExpr->expression->to<IR::PathExpression>()) {
        const auto *declInst = refMap.getDeclaration(implPath->path);
        if (declInst == nullptr) {
            ::error("Action profile reference %1% not found.", implPath->path->name);
            return std::nullopt;
        }
        implDecl = declInst;
    } else {
        ::error("Unimplemented action profile type %1%.", implExpr->expression->node_type_name());
        return std::nullopt;
    }

    return implDecl->controlPlaneName();
}

}  // namespace

bool TofinoControlPlaneInitializer::preorder(const IR::P4Table *table) {
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(*table, properties);
    auto tableName = table->controlPlaneName();

    ASSIGN_OR_RETURN(auto defaultActionConstraints, computeDefaultActionConstraints(table, refMap_),
                     false);
    ASSIGN_OR_RETURN(TableEntrySet initialTableEntries, initializeTableEntries(table, refMap_),
                     false);
    auto actionProfileNameOpt = checkForActionProfile(*table, refMap_);
    if (actionProfileNameOpt.has_value()) {
        auto actionProfileName = actionProfileNameOpt.value();
        ActionProfile *actionProfile = nullptr;
        auto it = defaultConstraints.find(actionProfileName);
        if (it != defaultConstraints.end()) {
            actionProfile = it->second.get().to<ActionProfile>();
            if (actionProfile == nullptr) {
                ::error(
                    "Looking up action profile %1%  in the constraints map did not return an "
                    "action profile.",
                    actionProfileName);
                return false;
            }
        } else {
            actionProfile = new ActionProfile();
            defaultConstraints.insert({actionProfileName, *actionProfile});
        }
        actionProfile->addAssociatedTable(tableName);
    }
    defaultConstraints.insert(
        {tableName, *new TableConfiguration(tableName, TableDefaultAction(defaultActionConstraints),
                                            initialTableEntries)});
    return false;
}

std::optional<ControlPlaneConstraints>
TofinoControlPlaneInitializer::generateInitialControlPlaneConstraints(
    const IR::P4Program *program) {
    program->apply(*this);
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    return getDefaultConstraints();
}

}  // namespace P4Tools::Flay::Tofino
