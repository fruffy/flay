#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_CONTROL_PLANE_ARCHITECTURE_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_TOFINO_CONTROL_PLANE_ARCHITECTURE_H_
#include "control-plane/p4RuntimeArchStandard.h"

namespace P4::ControlPlaneAPI::Standard {

/// Implements @ref P4RuntimeArchHandlerIface for the Tofino architecture. The
/// overridden methods will be called by the @P4RuntimeSerializer to collect and
/// serialize tofino-specific symbols which are exposed to the control-plane.
class P4RuntimeArchHandlerTofino final : public P4RuntimeArchHandlerCommon<Arch::PSA> {
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
