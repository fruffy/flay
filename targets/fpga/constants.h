#ifndef BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_CONSTANTS_H_
#define BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_CONSTANTS_H_

namespace P4Tools::Flay::Fpga {

class FpgaBaseConstants {
 public:
    /// Match bits exactly or not at all.
    static constexpr const char *MATCH_KIND_OPT = "optional";
    /// A match that is used as an argument for the selector.
    static constexpr const char *MATCH_KIND_SELECTOR = "selector";
    /// Entries that can match a range.
    static constexpr const char *MATCH_KIND_RANGE = "range";
};

}  // namespace P4Tools::Flay::Fpga

#endif /* BACKENDS_P4TOOLS_MODULES_FLAY_TARGETS_FPGA_CONSTANTS_H_ */
