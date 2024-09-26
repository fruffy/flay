
#include "backends/p4tools/modules/flay/targets/fpga/base/table_executor.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/modules/flay/core/interpreter/expression_resolver.h"
#include "backends/p4tools/modules/flay/targets/fpga/constants.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4::P4Tools::Flay::Fpga {

FpgaBaseTableExecutor::FpgaBaseTableExecutor(const IR::P4Table &table,
                                             ExpressionResolver &callingResolver)
    : TableExecutor(table, callingResolver) {}

const TableMatchKey *FpgaBaseTableExecutor::computeTargetMatchType(
    const IR::KeyElement *keyField) const {
    const auto *keyExpression = keyField->expression;
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
        return new OptionalMatchKey(symbolicTablePrefix(), fieldName, keyExpression);
    }
    // Action selector entries are not part of the match but we still need to create a key.
    if (matchType == FpgaBaseConstants::MATCH_KIND_SELECTOR) {
        return new SelectorMatchKey(symbolicTablePrefix(), fieldName, keyExpression);
    }
    if (matchType == FpgaBaseConstants::MATCH_KIND_RANGE) {
        return new RangeTableMatchKey(symbolicTablePrefix(), fieldName, keyExpression);
    }
    // If the custom match type does not match, delete to the core match types.
    return TableExecutor::computeTargetMatchType(keyField);
}

}  // namespace P4::P4Tools::Flay::Fpga
