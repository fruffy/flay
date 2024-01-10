#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_

#include <map>
#include <optional>

namespace P4Tools::Flay {

/// If @param expr evaluates to false, returns @param ret.
// NOLINTNEXTLINE
#define RETURN_IF_FALSE(expr, ret) \
    if (!(expr)) return ret;

/// If @param expr evaluates to false, returns @param ret and also executes @param err_fun.
// NOLINTNEXTLINE
#define RETURN_IF_FALSE_WITH_MESSAGE(expr, ret, err_fun) \
    if (!(expr)) {                                       \
        err_fun;                                         \
        return ret;                                      \
    }

/// Assigns the value of @param expr to @param lhs if the contained value is not false.
// NOLINTNEXTLINE
#define ASSIGN_OR_RETURN(lhs, expr, ret) \
    if (!(expr)) return ret;             \
    lhs = *(expr);  // NOLINT

/// Assigns the value of @param expr to @param lhs if the contained value is not false.
/// @param expr is dereferenced, which will work on std::nullopt and pointers, but not bools.
/// Prints an error provided with @param err_fun if the expr evaluates to false.
// NOLINTNEXTLINE
#define ASSIGN_OR_RETURN_WITH_MESSAGE(lhs, expr, ret, err_fun) \
    if (!(expr)) {                                             \
        err_fun;                                               \
        return ret;                                            \
    }                                                          \
    lhs = *(expr);  // NOLINT

/// Tries to look up a key in a map, returns std::nullopt if the key does not exist.
template <class K, class T, class V, class Comp, class Alloc>
inline std::optional<V> safeAt(const std::map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return std::nullopt;
}

}  // namespace P4Tools::Flay

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CONTROL_PLANE_UTIL_H_ */
