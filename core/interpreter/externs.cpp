#include "backends/p4tools/modules/flay/core/interpreter/externs.h"

#include <list>
#include <optional>
#include <tuple>
#include <vector>

#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/null.h"

namespace P4::P4Tools::Flay {

std::optional<ExternMethodImpls::MethodImpl> ExternMethodImpls::find(
    const IR::PathExpression &externObjectRef, const IR::ID &methodName,
    const IR::Vector<IR::Argument> *args) const {
    // We have to check the extern type here. We may receive a specialized canonical type, which we
    // need to unpack.
    const IR::Type_Extern *externType = nullptr;
    if (const auto *type = externObjectRef.type->to<IR::Type_Extern>()) {
        externType = type;
    } else if (const auto *specType = externObjectRef.type->to<IR::Type_SpecializedCanonical>()) {
        CHECK_NULL(specType->substituted);
        externType = specType->substituted->checkedTo<IR::Type_Extern>();
    } else if (externObjectRef.path->name == IR::ID("*method")) {
    } else {
        BUG("Not a valid extern: %1% with member %2%. Type is %3%.", externObjectRef, methodName,
            externObjectRef.type->node_type_name());
    }

    cstring qualifiedMethodName = externType->name + "." + methodName;
    if (impls.count(qualifiedMethodName) == 0) {
        return std::nullopt;
    }

    const auto &submap = impls.at(qualifiedMethodName);
    if (submap.count(args->size()) == 0) {
        return std::nullopt;
    }

    // Find matching methods: if any arguments are named, then the parameter name must match.
    std::optional<MethodImpl> matchingImpl;
    for (const auto &pair : submap.at(args->size())) {
        const auto &paramNames = pair.first;
        const auto &methodImpl = pair.second;

        if (matches(paramNames, args)) {
            BUG_CHECK(!matchingImpl, "Ambiguous extern method call: %1%", qualifiedMethodName);
            matchingImpl = methodImpl;
        }
    }

    return matchingImpl;
}

bool ExternMethodImpls::matches(const std::vector<cstring> &paramNames,
                                const IR::Vector<IR::Argument> *args) {
    CHECK_NULL(args);

    // Number of parameters should match the number of arguments.
    if (paramNames.size() != args->size()) {
        return false;
    }
    // Any named argument should match the name of the corresponding parameter.
    for (size_t idx = 0; idx < paramNames.size(); idx++) {
        const auto &paramName = paramNames.at(idx);
        const auto &arg = args->at(idx);

        if (arg->name.name == nullptr) {
            continue;
        }
        if (paramName != arg->name.name) {
            return false;
        }
    }

    return true;
}

ExternMethodImpls::ExternMethodImpls(const ImplList &inputImplList) {
    for (const auto &implSpec : inputImplList) {
        cstring name;
        std::vector<cstring> paramNames;
        MethodImpl impl;
        std::tie(name, paramNames, impl) = implSpec;

        auto &tmpImplList = impls[name][paramNames.size()];

        // Make sure that we have at most one implementation for each set of parameter names.
        // This is a quadratic-time algorithm, but should be fine, since we expect the number of
        // overloads to be small in practice.
        for (auto &pair : tmpImplList) {
            BUG_CHECK(pair.first != paramNames, "Multiple implementations of %1%(%2%)", name,
                      paramNames);
        }

        tmpImplList.emplace_back(paramNames, impl);
    }
}

}  // namespace P4::P4Tools::Flay
