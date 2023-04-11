#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_

#include <cstdint>
#include <set>
#include <string>

#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for P4Testgen.
class FlayOptions : public AbstractP4cToolOptions {
 public:
    virtual ~FlayOptions() = default;

    /// @returns the singleton instance of this class.
    static FlayOptions &get();

    /// Directory for the pruned program. Defaults to PWD.
    cstring outputDir = nullptr;

    const char *getIncludePath() override;

 private:
    FlayOptions();
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_ */
