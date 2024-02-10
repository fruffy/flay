# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify further.
p4tools_add_xfail_reason(
  "flay-tofino1-tna"
  "Compiler Bug|Unimplemented compiler support"
  tna_counter.p4  # direct_counter_0/direct_counter.count;: unknown type
  tna_checksum.p4  # Unknown or unimplemented extern method: ipv4_checksum.add
  tna_custom_hash.p4  # Unknown or unimplemented extern method: hash1.get
  tna_dkm.p4  # cntr_0/cntr.count;: unknown type
  tna_digest.p4  # Unknown or unimplemented extern method: digest_a.pack
  tna_dyn_hashing.p4  # Unknown or unimplemented extern method: hash_1.get
  tna_exact_match.p4  # cntr_0/cntr.count;: unknown type
  tna_meter_bytecount_adjust.p4  # meter_0/meter.execute;: unknown type
  tna_meter_lpf_wred.p4  # meter_0/meter.execute;: unknown type
  tna_operations.p4  # direct_counter_0/direct_counter.count;: unknown type
  tna_mirror.p4  # Unknown or unimplemented extern method: mirror.emit
  tna_port_metadata_extern.p4  # Unable to find var pkt; in the symbolic environment.
  tna_pvs.p4  # Unable to find var vs_0/vs; in the symbolic environment.
  tna_random.p4  # Unknown or unimplemented extern method: rnd1.get
  tna_register.p4  # Unknown or unimplemented extern method: test_reg_action.execute
  tna_resubmit.p4  # Unable to find var pkt; in the symbolic environment.
  tna_symmetric_hash.p4  # Unknown or unimplemented extern method: my_symmetric_hash.ge
  tna_32q_2pipe.p4  # meter_0/meter.execute;: unknown type
  tna_action_selector.p4  # Compiler Bug: No parameter named size
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



