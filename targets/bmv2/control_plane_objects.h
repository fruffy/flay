#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_CONTROL_PLANE_OBJECTS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_CONTROL_PLANE_OBJECTS_H_

#include <cstdint>

#include "backends/p4tools/modules/flay/core/control_plane/control_plane_item.h"
#include "lib/rtti.h"

namespace P4::P4Tools::Flay::V1Model {

/// BMv2-style clone session. If no session id is set, the clone session is considered inactive.
/// There should only be one clone session per program configuration.
class CloneSession : public Z3ControlPlaneItem {
    std::optional<uint32_t> _sessionId;

    ControlPlaneAssignmentSet _controlPlaneAssignments;
    Z3ControlPlaneAssignmentSet _z3ControlPlaneAssignments;

 public:
    explicit CloneSession(std::optional<uint32_t> sessionId) : _sessionId(sessionId) {}

    ~CloneSession() override = default;
    CloneSession(const CloneSession &) = default;
    CloneSession(CloneSession &&) = default;
    CloneSession &operator=(const CloneSession &) = default;
    CloneSession &operator=(CloneSession &&) = default;

    bool operator<(const ControlPlaneItem &other) const override;

    void setSessionId(uint32_t sessionId);

    [[nodiscard]] ControlPlaneAssignmentSet computeControlPlaneAssignments() const override;

    [[nodiscard]] Z3ControlPlaneAssignmentSet computeZ3ControlPlaneAssignments() const override;

    DECLARE_TYPEINFO(CloneSession);
};

}  // namespace P4::P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_CONTROL_PLANE_OBJECTS_H_ */
