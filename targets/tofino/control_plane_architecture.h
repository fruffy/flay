#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_CONTROL_PLANE_ARCHITECTURE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_CONTROL_PLANE_ARCHITECTURE_H_
#include "control-plane/p4RuntimeArchStandard.h"

namespace P4::ControlPlaneAPI::Helpers {
/// @ref CounterlikeTraits<> specialization for @ref CounterExtern for TNA
template <>
struct CounterlikeTraits<Standard::CounterExtern<Standard::Arch::TNA>> {
    static const cstring name() { return "counter"; }
    static const cstring directPropertyName() { return "tna_direct_counter"; }
    static const cstring typeName() { return "Counter"; }
    static const cstring directTypeName() { return "DirectCounter"; }
    static const cstring sizeParamName() { return "size"; }
    static p4configv1::CounterSpec::Unit mapUnitName(const cstring name) {
        using p4configv1::CounterSpec;
        if (name == "PACKETS")
            return CounterSpec::PACKETS;
        else if (name == "BYTES")
            return CounterSpec::BYTES;
        else if (name == "PACKETS_AND_BYTES")
            return CounterSpec::BOTH;
        return CounterSpec::UNSPECIFIED;
    }
    // the index of the type parameter for the counter index, in the type
    // parameter list of the extern type declaration.
    static std::optional<size_t> indexTypeParamIdx() { return 1; }
};

/// @ref CounterlikeTraits<> specialization for @ref MeterExtern for TNA
template <>
struct CounterlikeTraits<Standard::MeterExtern<Standard::Arch::TNA>> {
    static const cstring name() { return "meter"; }
    static const cstring directPropertyName() { return "tna_direct_meter"; }
    static const cstring typeName() { return "Meter"; }
    static const cstring directTypeName() { return "DirectMeter"; }
    static const cstring sizeParamName() { return "size"; }
    static p4configv1::MeterSpec::Unit mapUnitName(const cstring name) {
        using p4configv1::MeterSpec;
        if (name == "PACKETS")
            return MeterSpec::PACKETS;
        else if (name == "BYTES")
            return MeterSpec::BYTES;
        return MeterSpec::UNSPECIFIED;
    }
    // the index of the type parameter for the meter index, in the type
    // parameter list of the extern type declaration.
    static std::optional<size_t> indexTypeParamIdx() { return 0; }
};
}  // namespace P4::ControlPlaneAPI::Helpers

namespace P4::ControlPlaneAPI::Standard {
template <>
struct ActionProfileTraits<Arch::TNA> {
    static const cstring name() { return "action profile"; }
    static const cstring propertyName() { return "tna_implementation"; }
    static const cstring typeName() { return "ActionProfile"; }
    static const cstring sizeParamName() { return "size"; }
};

template <>
struct ActionSelectorTraits<Arch::TNA> : public ActionProfileTraits<Arch::TNA> {
    static const cstring name() { return "action selector"; }
    static const cstring typeName() { return "ActionSelector"; }
};

template <>
struct RegisterTraits<Arch::TNA> {
    static const cstring name() { return "register"; }
    static const cstring typeName() { return "Register"; }
    static const cstring sizeParamName() { return "size"; }
    static size_t dataTypeParamIdx() { return 0; }
    // the index of the type parameter for the register index, in the type
    // parameter list of the extern type declaration.
    static std::optional<size_t> indexTypeParamIdx() { return 1; }
};
}  // namespace P4::ControlPlaneAPI::Standard

namespace P4::ControlPlaneAPI::Standard {
/// Implements @ref P4RuntimeArchHandlerIface for the Tofino architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize tofino-specific symbols which are exposed to the control-plane.
class P4RuntimeArchHandlerTofino final : public P4RuntimeArchHandlerCommon<Arch::TNA> {
 public:
    P4RuntimeArchHandlerTofino(ReferenceMap *refMap, TypeMap *typeMap,
                               const IR::ToplevelBlock *evaluatedProgram);
};

/// The architecture handler builder implementation for Tofino.
struct TofinoArchHandlerBuilder : public P4RuntimeArchHandlerBuilderIface {
    P4RuntimeArchHandlerIface *operator()(ReferenceMap *refMap, TypeMap *typeMap,
                                          const IR::ToplevelBlock *evaluatedProgram) const override;
};

}  // namespace P4::ControlPlaneAPI::Standard

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_CONTROL_PLANE_ARCHITECTURE_H_ */
