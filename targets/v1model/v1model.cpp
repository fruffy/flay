#include "backends/p4tools/modules/flay/targets/v1model/v1model.h"

#include <string>

#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateTypedefs.h"
#include "midend/orderArguments.h"
#include "midend/removeExits.h"
#include "midend/removeLeftSlices.h"
#include "midend/simplifySelectList.h"

namespace P4Tools::Flay::V1Model {

V1ModelCompilerTarget::V1ModelCompilerTarget() : CompilerTarget("bmv2", "v1model") {}

void V1ModelCompilerTarget::make() {
    static V1ModelCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new V1ModelCompilerTarget();
    }
}
/// Implements the default enum-conversion policy, which converts all enums to bit<32>.
class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum * /*type*/) const override { return true; }

    [[nodiscard]] unsigned enumSize(unsigned /*enumCount*/) const override { return 32; }
};

class ErrorOn32Bits : public P4::ChooseErrorRepresentation {
    bool convert(const IR::Type_Error * /*type*/) const override { return true; }

    [[nodiscard]] unsigned errorSize(unsigned /*errorCount*/) const override { return 32; }
};

MidEnd V1ModelCompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    auto *refMap = midEnd.getRefMap();
    auto *typeMap = midEnd.getTypeMap();
    midEnd.addPasses({
        // Remove exit statements from the program.
        // TODO: We should not depend on this pass. It has bugs.
        new P4::RemoveExits(refMap, typeMap),
        // Replace types introduced by 'type' with 'typedef'.
        new P4::EliminateNewtype(refMap, typeMap),
        // Replace serializable enum constants with their values.
        new P4::EliminateSerEnums(refMap, typeMap),
        // Make sure that we have no TypeDef left in the program.
        new P4::EliminateTypedef(refMap, typeMap),
        // Sort call arguments according to the order of the function's parameters.
        new P4::OrderArguments(refMap, typeMap),
        new P4::ConvertEnums(refMap, typeMap, new EnumOn32Bits()),
        new P4::ConvertErrors(refMap, typeMap, new ErrorOn32Bits()),
        // Replace any slices in the left side of assignments and convert them to casts.
        new P4::RemoveLeftSlices(refMap, typeMap),
        // Flatten nested list expressions.
        new P4::SimplifySelectList(refMap, typeMap),
        // A final type checking pass to make sure everything is well-typed.
        new P4::TypeChecking(refMap, typeMap, true),
    });
    return midEnd;
}

}  // namespace P4Tools::Flay::V1Model
