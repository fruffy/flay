# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify.
p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Compiler Bug|Unimplemented compiler support"
  issue-2123-3-bmv2.p4  # Cannot match {(|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.srcAddr_0(bit<48>)| : 0)[7:0], |packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.etherType_0(bit<16>)| : 0}; with 2560 .. 2730;
  issue-2123.p4  # Cannot match {|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.etherType_0(bit<16>)| : 0, (|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.srcAddr_0(bit<48>)| : 0)[7:0], (|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.dstAddr_0(bit<48>)| : 0)[7:0]}; with 8 .. 16;
  issue1304.p4  # Cast failed: Pipeline<my_packet, my_metadata> with type Type_Specialized is not a Type_Declaration.
  invalid-hdr-warnings2.p4  # : No default value for type <Type_Unknown>(2) (Type_Unknown).
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
  issue1882-bmv2.p4  # Unknown or unimplemented extern method: extr.increment
  issue1882-1-bmv2.p4  # Unknown or unimplemented extern method: extr.increment
  issue3091.p4  # Unknown or unimplemented extern method: *method.fn_foo
  issue2664-bmv2.p4  # Unknown or unimplemented extern method: ipv4_checksum.update
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unable to find var .* in the symbolic environment"
  issue1955.p4  # Unable to find var p1_ipv4_ethertypes/ipv4_ethertypes; in the symbolic environment.
  pvs-bitstring-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-nested-struct.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-1-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-2-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-3-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  union-valid-bmv2.p4  # Unable to find var h.u.*valid; in the symbolic environment.
  v1model-p4runtime-enumint-types1.p4  # Unable to find var valueset1_0/valueset1; in the symbolic environment.
  v1model-p4runtime-most-types1.p4  # Unable to find var valueset1_0/valueset1; in the symbolic environment.
  flowlet_switching-bmv2.p4  # Unable to find var ecmp_base; in the symbolic environment.
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "expected a header or header union stack"
  issue4057.p4
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
