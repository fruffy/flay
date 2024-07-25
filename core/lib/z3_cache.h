#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_Z3_CACHE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_Z3_CACHE_H_

#include <z3++.h>

#include "absl/container/flat_hash_map.h"
#include "backends/p4tools/common/core/z3_solver.h"
#include "ir/ir.h"

namespace P4Tools {

/// A central cache to translate P4C expressions to Z3 expressions.
class Z3Cache : protected absl::flat_hash_map<const IR::Expression *, z3::expr> {
    Z3Solver _z3Solver;
    Z3Translator _z3Translator;

 private:
    Z3Cache() : _z3Translator(_z3Solver) {}

    static Z3Cache &get() {
        static Z3Cache Z3_CACHE;
        return Z3_CACHE;
    }

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

    std::optional<z3::expr> getImpl(const IR::Expression *expression) const {
        auto it = find(expression);
        if (it != end()) {
            return it->second;
        }
        return std::nullopt;
    }

    void removeImpl(const IR::Expression *expression) { erase(expression); }

 public:
    static std::optional<z3::expr> get(const IR::Expression *expression) {
        return get().getImpl(expression);
    }

    static z3::expr set(const IR::Expression *expression) { return get().setImpl(expression); }

    static void remove(const IR::Expression *expression) { get().removeImpl(expression); }
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_CORE_LIB_Z3_CACHE_H_ */
