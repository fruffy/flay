# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify.
p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Compiler Bug|Unimplemented compiler support"
  issue1304.p4 # Cast failed: Pipeline<my_packet, my_metadata> with type Type_Specialized is not a
               # Type_Declaration.
  header-stack-ops-bmv2.p4 # Unknown method member expression: hdr_0.h2; of type header h2_t
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unsupported assignment"
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unknown or unimplemented extern method: .*"
)

# These are custom externs we do not implement.
p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unknown or unimplemented extern method: .*"
  issue1882-bmv2.p4 # Unknown or unimplemented extern method: extr.increment
  issue1882-1-bmv2.p4 # Unknown or unimplemented extern method: extr.increment
  issue2664-bmv2.p4 # Unknown or unimplemented extern method: ipv4_checksum.update
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unable to find var .* in the symbolic environment"
  # Most of these are value sets, which we do not support.
  issue1955.p4 # Unable to find var p1_ipv4_ethertypes/ipv4_ethertypes; in the symbolic environment.
  pvs-bitstring-bmv2.p4 # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-nested-struct.p4 # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-1-bmv2.p4 # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-2-bmv2.p4 # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-3-bmv2.p4 # Unable to find var pvs_0/pvs; in the symbolic environment.
  union-valid-bmv2.p4 # Unable to find var h.u.*valid; in the symbolic environment.
  v1model-p4runtime-enumint-types1.p4 # Unable to find var valueset1_0/valueset1; in the symbolic
                                      # environment.
  v1model-p4runtime-most-types1.p4 # Unable to find var valueset1_0/valueset1; in the symbolic
                                   # environment.
  issue3091.p4 # WONTFIX Unable to find var ternary; in the symbolic environment.
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "expected a header or header union stack"
  issue4057.p4
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model" "Only constants are supported"
  parser-unroll-test10.p4 # Value meta.hs_next_index; is not a constant. Only constants are
                          # supported as part of a state variable.
                          # hdr.hs[meta.hs_next_index].setValid();
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unsupported type argument for Value Set"
  pvs-nested-struct.p4
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Parser state .* was already visited. We currently do not support parser loops."
  issue2314.p4
  invalid-hdr-warnings1.p4
  parser-unroll-test9.p4
  parser-unroll-issue3537.p4
  parser-unroll-issue3537-1.p4
  issue281.p4
  fabric.p4
)

# When trying to remove dead code we can not find a particular node in the reachability map.
# Often this happens because the compiler optimizes the expression away and Flay never sees it.
p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "error: Unable to find node .* in the reachability map"
  issue2345-multiple_dependencies.p4
  issue1765-1-bmv2.p4
  issue2345-with_nested_if.p4
  gauntlet_exit_combination_1-bmv2.p4
  gauntlet_exit_combination_11-bmv2.p4
  gauntlet_exit_combination_16-bmv2.p4
  issue420.p4
)

# We are trying to map a duplicate condition to the reachability map.
# This can happen when the source information is ambiguous.
# Unclear how to resolve this issue as it emerges from unclean compiler passes.
p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Reachability mapping for node .* already exists"
  control-hs-index-test2.p4
  control-hs-index-test3.p4
  control-hs-index-test4.p4
  issue1127-bmv2.p4
  issue2344.p4
  issue512.p4
  xor_test.p4
)


