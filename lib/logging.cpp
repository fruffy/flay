
#include "backends/p4tools/modules/flay/lib/logging.h"

#include "lib/log.h"

namespace P4Tools::Flay {

void enableInformationLogging() { Log::addDebugSpec("test_info:4"); }

}  // namespace P4Tools::Flay
