#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STATE_UTILS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STATE_UTILS_H_

#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "backends/p4tools/modules/flay/core/program_info.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace P4Tools::Flay::StateUtils {

/// @returns an IR::Expression converted into a StateVariable. Currently only IR::PathExpression
/// and IR::Member can be converted into a state variable.
IR::StateVariable convertReference(const IR::Expression *ref);

/// Looks up the @param member in the environment of @param state. Returns nullptr if the member
/// is not a table type.
const IR::P4Table *findTable(const ExecutionState &state, const IR::Member *member);

/// Takes an input struct type @ts and a prefix @parent and returns a vector of references to
/// members of the struct. The vector contains all the Type_Base (bit and bool) members in
/// canonical representation (e.g.,
/// {"prefix.h.ethernet.dst_address", "prefix.h.ethernet.src_address", ...}). If @arg
/// validVector is provided, this function also collects the validity bits of the headers.
[[nodiscard]] std::vector<IR::StateVariable> getFlatFields(
    ExecutionState &state, const IR::Expression *parent, const IR::Type_StructLike *ts,
    std::vector<IR::StateVariable> *validVector = nullptr);

/// Takes an input struct expression @se and returns a flattened vector of all the Type_Base
/// (bit and bool) expressions contained within the struct expression.
[[nodiscard]] std::vector<const IR::Expression *> getFlatStructFields(
    const IR::StructExpression *se);

/// Initialize all the members of a struct-like object by calling the initialization function of
/// the active target. Headers validity is set to "false".
void initializeStructLike(const ProgramInfo &programInfo, ExecutionState &state,
                          const IR::Expression *target, bool forceTaint);

/// Set the members of struct-like @target with the values of struct-like @source.
void setStructLike(ExecutionState &state, const IR::Expression *target,
                   const IR::Expression *source);

/// Initialize a Declaration_Variable to its default value. Does not expect an initializer.
void declareVariable(const ProgramInfo &programInfo, ExecutionState &state,
                     const IR::Declaration_Variable &declVar);

/// Copy the values referenced by @param @externalParamName into the values references by @param
/// @internalParam. Any parameter with the direction "out" is de-initialized.
void copyIn(ExecutionState &executionState, const ProgramInfo &programInfo,
            const IR::Parameter *internalParam, cstring externalParamName);

/// Copy the values referenced by @param @internalParam into the values references by @param
/// @externalParamName. Only parameters with the direction out or inout are copied.
void copyOut(ExecutionState &executionState, const IR::Parameter *internalParam,
             cstring externalParamName);

/// Initialize a set of parameters contained in @param @blockParams to their default values. The
/// default values are specified by the target.
void initializeBlockParams(const ProgramInfo &programInfo, ExecutionState &state,
                           const IR::Type_Declaration *typeDecl,
                           const std::vector<cstring> *blockParams);

/// Try to extract and @return the IR::P4Action from the action call. This is done by looking up the
/// reference in the execution state. Throws a BUG, if the action does not exist.
[[nodiscard]] const IR::P4Action *getP4Action(ExecutionState &state,
                                              const IR::MethodCallExpression *actionExpr);

}  // namespace P4Tools::Flay::StateUtils

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_STATE_UTILS_H_ */
