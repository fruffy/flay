#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_STATE_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_STATE_UTILS_H_

#include <functional>
#include <vector>

#include "backends/p4tools/common/lib/variables.h"
#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4Tools::Flay {

class StateUtils {
 public:
    static IR::StateVariable convertReference(const IR::Expression *ref);

    /// Takes an input struct type @ts and a prefix @parent and appends each
    /// field of the struct type to the provided vector @flatFields. The result
    /// is a vector of all in the bit and bool members in canonical
    /// representation (e.g., {"prefix.h.ethernet.dst_address",
    /// "prefix.h.ethernet.src_address", ...}).
    /// If @arg validVector is provided, this function also collects the validity
    /// bits of the headers.
    [[nodiscard]] static std::vector<const IR::Member *> getFlatFields(
        ExecutionState &state, const IR::Expression *parent, const IR::Type_StructLike *ts,
        std::vector<const IR::Member *> *validVector = nullptr);

    [[nodiscard]] static std::vector<const IR::Expression *> getFlatStructFields(
        const IR::StructExpression *se);

    static void initializeStructLike(const ProgramInfo &programInfo, ExecutionState &state,
                                     const IR::Expression *target, bool forceTaint);

    static void setStructLike(ExecutionState &state, const IR::Expression *target,
                              const IR::Expression *source);

    static void copyIn(ExecutionState &executionState, const ProgramInfo &programInfo,
                       const IR::Parameter *internalParam, cstring externalParamName);

    void static copyOut(ExecutionState &executionState, const IR::Parameter *internalParam,
                        cstring externalParamName);

    static void initializeBlockParams(const ProgramInfo &programInfo, ExecutionState &state,
                                      const IR::Type_Declaration *typeDecl,
                                      const std::vector<cstring> *blockParams);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_STATE_UTILS_H_ */
