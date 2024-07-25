#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_Z3_CACHE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_Z3_CACHE_H_

#include <z3++.h>

#include "absl/container/flat_hash_map.h"
#include "backends/p4tools/common/core/z3_solver.h"
#include "ir/ir.h"

namespace P4Tools {

/// Memoizes P4C expressions which have been translated to Z3 expressions.
class Z3Cache : protected absl::flat_hash_map<const IR::Expression *, z3::expr> {
    /// The Z3 solver.
    Z3Solver _z3Solver;

    /// The Z3 translator.
    Z3Translator _z3Translator;

 private:
    Z3Cache() : _z3Translator(_z3Solver) {}

    /// The cache is a singleton instance.
    static Z3Cache &getInstance() {
        static Z3Cache Z3_CACHE;
        return Z3_CACHE;
    }

    /// See @set.
    z3::expr setImpl(const IR::Expression *expression) {
        auto it = find(expression);
        // Already set, nothing to do here.
        if (it != end()) {
            return it->second;
        }
        auto result = _z3Translator.translate(expression).simplify();
        insert({expression, result});
        return result;
    }

    /// See @get.
    std::optional<z3::expr> getImpl(const IR::Expression *expression) const {
        auto it = find(expression);
        if (it != end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// See @remove.
    void removeImpl(const IR::Expression *expression) { erase(expression); }

 public:
    /// Return the memoized Z3 expression for the provided expression.
    static std::optional<z3::expr> get(const IR::Expression *expression) {
        return getInstance().getImpl(expression);
    }

    /// Translate the provided expression, memoize the result, and return it.
    static z3::expr set(const IR::Expression *expression) {
        return getInstance().setImpl(expression);
    }

    /// Remove the provided expression from the cache.
    static void remove(const IR::Expression *expression) { getInstance().removeImpl(expression); }

    /// Return the underlying Z3 context.
    static z3::context &context() { return getInstance()._z3Solver.mutableContext(); }
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_Z3_CACHE_H_ */
