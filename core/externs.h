#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXTERNS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXTERNS_H_

#include <sys/types.h>

#include <functional>
#include <list>
#include <map>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "backends/p4tools/modules/flay/core/execution_state.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"

namespace P4Tools::Flay {

/// Encapsulates a set of extern method implementations.
class ExternMethodImpls {
 public:
    /// The type of an extern-method implementation. See @ref exec for an explanation of the
    /// arguments.
    using MethodImpl =
        std::function<const IR::Expression *(const IR::PathExpression &, const IR::ID &,
                                             const IR::Vector<IR::Argument> *, ExecutionState &)>;
    /// Evaluates a call to an extern method. Upon return, the given result will be augmented with
    /// the successor states resulting from evaluating the call.
    ///
    /// @param call the original method call expression, can be used for stepInto calls.
    /// @param receiver a symbolic value representing the object on which the method is being
    ///     called.
    /// @param name the name of the method being called.
    /// @param args the list of arguments being passed to method.
    /// @param state the state in which the call is being made, with the call at the top of the
    ///     current continuation body.
    ///
    /// @returns true if a matching extern-method implementation was found; false otherwise.
    ///
    /// A BUG occurs if the receiver is not an extern or if the call is ambiguous.
    std::optional<MethodImpl> find(const IR::PathExpression &externObjectRef,
                                   const IR::ID &methodName,
                                   const IR::Vector<IR::Argument> *args) const;

 private:
    /// A two-level map. First-level keys are method names qualified by the extern type name (e.g.,
    /// "packet_in.advance". Second-level keys are the number of arguments. Values are lists of
    /// matching extern-method implementations, paired with the names of the method parameters.
    std::map<cstring, std::map<uint, std::list<std::pair<std::vector<cstring>, MethodImpl>>>> impls;

    /// Determines whether the given list of parameter names matches the given argument list.
    /// According to the P4 specification, a match occurs when the lists have the same length and
    /// the name of any named argument matches the name of the corresponding parameter.
    static bool matches(const std::vector<cstring> &paramNames,
                        const IR::Vector<IR::Argument> *args);

 public:
    /// Represents a list of extern method implementations. The components of each element in this
    /// list are as follows:
    ///   1. The method name qualified by the extern type name (e.g., "packet_in.advance").
    ///   2. The names of the method parameters.
    ///   3. The method's implementation.
    using ImplList = std::list<std::tuple<cstring, std::vector<cstring>, MethodImpl>>;

    explicit ExternMethodImpls(const ImplList &implList);
};

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_EXTERNS_H_ */
