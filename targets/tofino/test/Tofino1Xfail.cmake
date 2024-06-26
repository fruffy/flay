# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify further.
p4tools_add_xfail_reason(
  "flay-tofino1-tna"
  "Compiler Bug|Unimplemented compiler support"
  tna_meter_lpf_wred.p4  # Unimplemented extern method: simple_lpf.execute
  tna_mirror.p4  # Unimplemented extern method: mirror.emit
  tna_resubmit.p4  # Unimplemented extern method: resubmit.emit
  tna_32q_2pipe.p4  # The Tofino1 architecture requires 6 pipes. Received 12.
)

p4tools_add_xfail_reason(
  "flay-tofino1-tna"
  "Match type .* not implemented for table keys"
  tna_ternary_match.p4  # error: Match type atcam_partition_index not implemented for table keys
)

# When trying to remove dead code we can not find a particular node in the reachability map.
# Often this happens because the compiler optimizes the expression away and Flay never sees it.
p4tools_add_xfail_reason(
  "flay-tofino1-tna"
  "Unable to find node .* in the reachability map of this execution state"
)



