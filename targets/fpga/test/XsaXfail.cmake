# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify further.
p4tools_add_xfail_reason(
  "flay-fpga-xsa"
  "Compiler Bug|Unimplemented compiler support"
)

p4tools_add_xfail_reason(
  "flay-fpga-xsa"
  "Match type .* not implemented for table keys"
)

# When trying to remove dead code we can not find a particular node in the reachability map.
# Often this happens because the compiler optimizes the expression away and Flay never sees it.
p4tools_add_xfail_reason(
  "flay-fpga-xsa"
  "Unable to find node .* in the reachability map of this execution state"
)



