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
                                                 cstring matchType,
                                                 ControlPlaneAssignmentSet &keySet) {
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

std::optional<cstring> associateActionProfiles(const IR::P4Table &table,
                                               const P4::ReferenceMap &refMap,
                                               ControlPlaneConstraints &_defaultConstraints) {
    const auto *impl = table.properties->getProperty(cstring("implementation"));
    if (impl == nullptr) {
        return std::nullopt;
    }

    const auto *implExpr = impl->value->checkedTo<IR::ExpressionValue>();
    const IR::IDeclaration *implementationDeclaration = nullptr;
    const IR::IDeclaration *implementationTypeDeclaration = nullptr;

    if (const auto *implPath = implExpr->expression->to<IR::PathExpression>()) {
        const auto *decl = refMap.getDeclaration(implPath->path);
        if (decl == nullptr) {
            ::error("Action profile reference %1% not found.", implPath->path->name);
            return std::nullopt;
        }
        const auto *declarationInstance = decl->to<IR::Declaration_Instance>();
        if (declarationInstance == nullptr) {
            ::error("Action profile reference %1% is not an instance.", implPath->path->name);
            return std::nullopt;
        }
        const auto *declarationInstanceTypeName = declarationInstance->type->to<IR::Type_Name>();
        if (declarationInstanceTypeName == nullptr) {
            ::error("Implementation instance type %1% is not a type name.", implPath->path->name);
            return std::nullopt;
        }
        implementationTypeDeclaration = refMap.getDeclaration(declarationInstanceTypeName->path);
        implementationDeclaration = declarationInstance;
    } else {
        ::error("Unimplemented action profile type %1%.", implExpr->expression->node_type_name());
        return std::nullopt;
    }
    auto declarationControlPlaneName = implementationDeclaration->controlPlaneName();
    if (implementationTypeDeclaration->getName() == "ActionSelector") {
        ActionSelector *actionSelector = nullptr;
        auto it = _defaultConstraints.find(declarationControlPlaneName);
        if (it != _defaultConstraints.end()) {
            actionSelector = it->second.get().to<ActionSelector>();
        }
        if (actionSelector == nullptr) {
            ::error("Could not find action selector %1%.", declarationControlPlaneName);
            return std::nullopt;
        }
        actionSelector->addAssociatedTable(table.controlPlaneName());
        return actionSelector->actionProfile().name();
    }
    if (implementationTypeDeclaration->getName() == "ActionProfile") {
        ActionProfile *actionProfile = nullptr;
        auto it = _defaultConstraints.find(declarationControlPlaneName);
        if (it != _defaultConstraints.end()) {
            actionProfile = it->second.get().to<ActionProfile>();
        }
        if (actionProfile == nullptr) {
            ::error("Could not find action profile %1%.", declarationControlPlaneName);
            return std::nullopt;
        }
        actionProfile->addAssociatedTable(table.controlPlaneName());
        return actionProfile->name();
    }
    if (implementationTypeDeclaration->getName() == "Alpm") {
        ::warning("Implementation type %1% is not yet supported for tables.",
                  implementationTypeDeclaration);
        return std::nullopt;
    }
    ::error("Implementation type %1% is not an action selector or action profile.",
            implementationTypeDeclaration->getName());
    return std::nullopt;
}

}  // namespace

bool TofinoControlPlaneInitializer::preorder(const IR::Declaration_Instance *declaration) {
    const auto *declarationInstanceTypeName = declaration->type->to<IR::Type_Name>();
    if (declarationInstanceTypeName == nullptr) {
        return false;
    }
    const auto *implTypeDeclaration = refMap().getDeclaration(declarationInstanceTypeName->path);
    if (implTypeDeclaration == nullptr) {
        return false;
    }

    if (implTypeDeclaration->getName() == "ActionProfile") {
        _defaultConstraints.insert(
            {declaration->controlPlaneName(), *new ActionProfile(declaration->controlPlaneName())});
    }
    if (implTypeDeclaration->getName() == "ActionSelector") {
        const auto *declarationArguments = declaration->arguments;
        constexpr int kMinimumExpectedArgumentSize = 5;
        if (declarationArguments->size() < kMinimumExpectedArgumentSize) {
            ::error("Action selector %1% requires %2% arguments, but only %3% were provided.",
                    implTypeDeclaration->controlPlaneName(), kMinimumExpectedArgumentSize,
                    declarationArguments->size());
            return false;
        }
        const auto *actionProfileReference = declarationArguments->at(0)->expression;
        const auto *actionProfileReferencePath = actionProfileReference->to<IR::PathExpression>();
        if (actionProfileReferencePath == nullptr) {
            ::error("Action profile reference %1% is not a path expression.",
                    actionProfileReference);
            return false;
        }
        const auto *actionProfileDeclaration =
            refMap().getDeclaration(actionProfileReferencePath->path);
        auto actionProfileName = actionProfileDeclaration->controlPlaneName();
        ActionProfile *actionProfileObject = nullptr;
        auto constraintObject = _defaultConstraints.find(actionProfileName);
        if (constraintObject != _defaultConstraints.end()) {
            actionProfileObject = constraintObject->second.get().to<ActionProfile>();
        }
        if (actionProfileObject == nullptr) {
            ::error(
                "The action selector must reference an existing action profile but action profile "
                "%1% was not found.",
                actionProfileName);
            return false;
        }
        _defaultConstraints.insert(
            {declaration->controlPlaneName(), *new ActionSelector(*actionProfileObject)});
    }

    return false;
}

bool TofinoControlPlaneInitializer::preorder(const IR::P4Table *table) {
    TableUtils::TableProperties properties;
    TableUtils::checkTableImmutability(*table, properties);
    auto tableName = table->controlPlaneName();

    // TODO: Consume the returned value here?
    associateActionProfiles(*table, refMap(), _defaultConstraints);

    ASSIGN_OR_RETURN(auto defaultActionConstraints,
                     computeDefaultActionConstraints(table, refMap()), false);

    ASSIGN_OR_RETURN(TableEntrySet initialTableEntries, initializeTableEntries(table, refMap()),
                     false);
    auto config =
        *new TableConfiguration(table->controlPlaneName(),
                                TableDefaultAction(defaultActionConstraints), initialTableEntries);
    _defaultConstraints.insert(
        {tableName, *new TableConfiguration(table->controlPlaneName(),
                                            TableDefaultAction(defaultActionConstraints),
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
    return defaultConstraints();
}

}  // namespace P4Tools::Flay::Tofino
