#include "backends/p4tools/modules/flay/targets/v1model/v1model.h"

#include <string>

namespace P4Tools::Flay::V1Model {

V1ModelCompilerTarget::V1ModelCompilerTarget() : CompilerTarget("bmv2", "v1model") {}

void V1ModelCompilerTarget::make() {
    static V1ModelCompilerTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new V1ModelCompilerTarget();
    }
}

MidEnd V1ModelCompilerTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);

    return midEnd;
}

}  // namespace P4Tools::Flay::V1Model
