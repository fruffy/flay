#include "backends/p4tools/modules/flay/targets/tofino/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/util.h"
#include "backends/p4tools/modules/flay/targets/tofino/constants.h"

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
            keySet.emplace(*minKey, *IR::getConstant(keySymbol.type, 0));
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
