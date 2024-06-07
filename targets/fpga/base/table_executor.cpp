
#include "backends/p4tools/modules/flay/targets/fpga/base/table_executor.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/fpga/constants.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::Flay::Fpga {

FpgaBaseTableExecutor::FpgaBaseTableExecutor(const IR::P4Table &table,
                                             ExpressionResolver &callingResolver)
    : TableExecutor(table, callingResolver) {}

const IR::Expression *FpgaBaseTableExecutor::computeTargetMatchType(
    const IR::KeyElement *keyField) const {
    auto tableName = getP4Table().controlPlaneName();
    const auto *keyExpr = keyField->expression;
    const auto matchType = keyField->matchType->toString();
    const auto *nameAnnot = keyField->getAnnotation(IR::Annotation::nameAnnotation);
    bool isTainted = false;
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
        cstring keyName = tableName + "_key_" + fieldName;
        const auto *ctrlPlaneKey = ToolsVariables::getSymbolicVariable(keyExpr->type, keyName);
        if (isTainted) {
            return IR::BoolLiteral::get(true);
        }
        return new IR::Equ(keyExpr, ctrlPlaneKey);
    }
    // Action selector entries are not part of the match.
    if (matchType == FpgaBaseConstants::MATCH_KIND_SELECTOR) {
        return IR::BoolLiteral::get(true);
    }
    if (matchType == FpgaBaseConstants::MATCH_KIND_RANGE) {
        cstring minName = tableName + "_range_min_" + fieldName;
        cstring maxName = tableName + "_range_max_" + fieldName;
        // We can recover from taint by matching on the entire possible range.
        const IR::Expression *minKey = nullptr;
        const IR::Expression *maxKey = nullptr;
        if (isTainted) {
            minKey = IR::Constant::get(keyExpr->type, 0);
            maxKey = IR::Constant::get(keyExpr->type, IR::getMaxBvVal(keyExpr->type));
            keyExpr = minKey;
        } else {
            minKey = ToolsVariables::getSymbolicVariable(keyExpr->type, minName);
            maxKey = ToolsVariables::getSymbolicVariable(keyExpr->type, maxName);
        }
        return new IR::LAnd(new IR::LAnd(new IR::Lss(minKey, maxKey), new IR::Leq(minKey, keyExpr)),
                            new IR::Leq(keyExpr, maxKey));
    }
    // If the custom match type does not match, delete to the core match types.
    return TableExecutor::computeTargetMatchType(keyField);
}

}  // namespace P4Tools::Flay::Fpga
