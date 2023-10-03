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
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unsupported assignment"
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unknown or unimplemented extern method: .*"
  pins_fabric.p4  # Unknown or unimplemented extern method: *method.clone
  pins_middleblock.p4  # Unknown or unimplemented extern method: *method.clone
  pins_wbb.p4  # Unknown or unimplemented extern method: *method.clone
  issue1001-1-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1001-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1642-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1653-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1660-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1765-1-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue383-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue562-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1653-complex-bmv2.p4  # Unknown or unimplemented extern method: *method.clone_preserving_field_list
  issue1043-bmv2.p4  # Unknown or unimplemented extern method: *method.resubmit_preserving_field_list
  v1model-special-ops-bmv2.p4  # Unknown or unimplemented extern method: *method.resubmit_preserving_field_list
  equality-bmv2.p4  # Unknown or unimplemented extern method: b.extract
  equality-varbit-bmv2.p4  # Unknown or unimplemented extern method: b.extract
  issue1879-bmv2.p4  # Unknown or unimplemented extern method: packet.extract
  issue447-1-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue447-2-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue447-3-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue447-4-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue447-5-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  test-parserinvalidargument-error-bmv2.p4  # Unknown or unimplemented extern method: packet.extract
  issue447-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  checksum-l4-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  checksum1-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue1025-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue1560-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue1765-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue3702-bmv2.p4  # Unknown or unimplemented extern method: pkt.extract
  issue561-bmv2.p4  # Unknown or unimplemented extern method: packet.extract
  dash-pipeline.p4  # Unknown or unimplemented extern method: packet.extract
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
  array-copy-bmv2.p4  # Unable to find var h.h1; in the symbolic environment.
  gauntlet_hdr_set_valid-bmv2.p4  # Unable to find var local_h_0/local_h; in the symbolic environment.
  issue1210.p4  # Unable to find var meta.foo; in the symbolic environment.
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
  issue2201-bmv2.p4  # Unable to find var hdr.ethernet; in the symbolic environment.
  issue3001-1.p4  # Unable to find var s_0/s; in the symbolic environment.
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
