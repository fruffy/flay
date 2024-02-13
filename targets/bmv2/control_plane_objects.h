#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_CONTROL_PLANE_OBJECTS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_CONTROL_PLANE_OBJECTS_H_

#include <cstdint>

#include "backends/p4tools/modules/flay/control_plane/control_plane_item.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/rtti.h"

namespace P4Tools::Flay::V1Model {

/// BMv2-style clone session. If no session id is set, the clone session is considered inactive.
/// There should only be one clone session per program configuration.
class CloneSession : public ControlPlaneItem {
    std::optional<uint32_t> sessionId;

 public:
    explicit CloneSession(std::optional<uint32_t> sessionId) : sessionId(sessionId) {}

    ~CloneSession() override = default;
    CloneSession(const CloneSession &) = default;
    CloneSession(CloneSession &&) = default;
    CloneSession &operator=(const CloneSession &) = default;
    CloneSession &operator=(CloneSession &&) = default;

    bool operator<(const ControlPlaneItem &other) const override;

    void setSessionId(uint32_t sessionId);

    [[nodiscard]] const IR::Expression *computeControlPlaneConstraint() const override;

    DECLARE_TYPEINFO(CloneSession);
};

}  // namespace P4Tools::Flay::V1Model

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_BMV2_CONTROL_PLANE_OBJECTS_H_ */
