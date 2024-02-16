#include "backends/p4tools/modules/flay/targets/tofino/control_plane_architecture.h"

namespace P4::ControlPlaneAPI::Standard {

P4RuntimeArchHandlerTofino::P4RuntimeArchHandlerTofino(ReferenceMap *refMap, TypeMap *typeMap,
                                                       const IR::ToplevelBlock *evaluatedProgram)
    : P4RuntimeArchHandlerCommon<Arch::TNA>(refMap, typeMap, evaluatedProgram) {}

P4RuntimeArchHandlerIface *TofinoArchHandlerBuilder::operator()(
    ReferenceMap *refMap, TypeMap *typeMap, const IR::ToplevelBlock *evaluatedProgram) const {
    return new P4RuntimeArchHandlerTofino(refMap, typeMap, evaluatedProgram);
};

}  // namespace P4::ControlPlaneAPI::Standard
