#include "backends/p4tools/modules/flay/targets/fpga/symbolic_state.h"

#include <cstdlib>
#include <optional>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/control_plane/return_macros.h"
#include "backends/p4tools/modules/flay/targets/fpga/constants.h"

namespace P4Tools::Flay::Fpga {

bool FpgaControlPlaneInitializer::computeMatch(const IR::Expression &entryKey,
                                               const IR::SymbolicVariable &keySymbol,
                                               cstring tableName, cstring fieldName,
                                               cstring matchType,
                                               ControlPlaneAssignmentSet &keySet) {
    if (matchType == FpgaBaseConstants::MATCH_KIND_RANGE) {
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
    if (matchType == FpgaBaseConstants::MATCH_KIND_OPT) {
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
FpgaControlPlaneInitializer::generateInitialControlPlaneConstraints(const IR::P4Program *program) {
    program->apply(*this);
    if (::errorCount() > 0) {
        return std::nullopt;
    }
    return defaultConstraints();
}

}  // namespace P4Tools::Flay::Fpga
