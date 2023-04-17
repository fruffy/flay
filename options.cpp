#include "backends/p4tools/modules/flay/options.h"

#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

FlayOptions &FlayOptions::get() {
    static FlayOptions INSTANCE;
    return INSTANCE;
}

const char *FlayOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath not implemented for Flay.");
}

FlayOptions::FlayOptions()
    : AbstractP4cToolOptions("Remove control-plane dead code from a P4 program.") {}

}  // namespace P4Tools
