
#include "backends/p4tools/modules/flay/targets/fpga/base/table_executor.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/control_plane/symbolic_variables.h"
#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/fpga/constants.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay::Fpga {

FpgaBaseTableExecutor::FpgaBaseTableExecutor(const IR::P4Table &table,
                                             ExpressionResolver &callingResolver)
    : TableExecutor(table, callingResolver) {}

const TableMatchKey *FpgaBaseTableExecutor::computeTargetMatchType(
    const IR::KeyElement *keyField) const {
    auto tableName = getP4Table().controlPlaneName();
    const auto *keyExpr = keyField->expression;
    const auto matchType = keyField->matchType->toString();
    const auto *nameAnnot = keyField->getAnnotation(IR::Annotation::nameAnnotation);
    // Some hidden tables do not have any key name annotations.
    BUG_CHECK(nameAnnot != nullptr /* || properties.tableIsImmutable*/,
              "Non-constant table key without an annotation");
    cstring fieldName;
    if (nameAnnot != nullptr) {
        fieldName = nameAnnot->getName();
    }

    // TODO: We consider optional match types to be a no-op, but we could make them exact matches.
    if (matchType == FpgaBaseConstants::MATCH_KIND_OPT) {
        // We can recover from taint by simply not adding the optional match.
        // Create a new symbolic variable that corresponds to the key expression.
        const auto *ctrlPlaneKey =
            ControlPlaneState::getTableKey(tableName, fieldName, keyExpr->type);
        return new OptionalMatchKey(fieldName, ctrlPlaneKey, keyExpr);
    }
    // Action selector entries are not part of the match but we still need to create a key.
    if (matchType == FpgaBaseConstants::MATCH_KIND_SELECTOR) {
        cstring keyName = tableName + "_selector_" + fieldName;
        const auto *ctrlPlaneKey = ToolsVariables::getSymbolicVariable(keyExpr->type, keyName);
        return new SelectorMatchKey(keyName, ctrlPlaneKey, keyExpr);
    }
    if (matchType == FpgaBaseConstants::MATCH_KIND_RANGE) {
        // We can recover from taint by matching on the entire possible range.
        auto [minKey, maxKey] =
            Bmv2ControlPlaneState::getTableRange(tableName, fieldName, keyExpr->type);
        return new RangeTableMatchKey(fieldName, minKey, maxKey, keyExpr);
    }
    // If the custom match type does not match, delete to the core match types.
    return TableExecutor::computeTargetMatchType(keyField);
}

}  // namespace P4Tools::Flay::Fpga
