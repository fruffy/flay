#include "backends/p4tools/modules/flay/options.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/options.h"
#include "lib/error.h"
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
    : AbstractP4cToolOptions("Remove control-plane dead code from a P4 program.") {
    registerOption(
        "--out-dir", "outputDir",
        [this](const char *arg) {
            outputDir = arg;
            return true;
        },
        "The output directory for the generated tests. The directory will be created, if it does "
        "not exist.");
}

}  // namespace P4Tools
