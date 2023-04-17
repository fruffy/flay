#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_

#include "backends/p4tools/common/options.h"

namespace P4Tools {

/// Encapsulates and processes command-line options for P4Testgen.
class FlayOptions : public AbstractP4cToolOptions {
 public:
    virtual ~FlayOptions() = default;

    /// @returns the singleton instance of this class.
    static FlayOptions &get();

    const char *getIncludePath() override;

 private:
    FlayOptions();
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_OPTIONS_H_ */
