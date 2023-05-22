# XFAILS: tests that currently fail. Most of these are temporary.
# ================================================

# Wildcard failing tests for now. TODO:  Classify.
p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Compiler Bug|Unimplemented compiler support"
  parser-unroll-test1.p4  # Cast failed: hdr.srcRoutes[tmp/index]; with type ArrayIndex is not a PathExpression.
  issue-2123-3-bmv2.p4  # Cannot match {(|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.srcAddr_0(bit<48>)| : 0)[7:0], |packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.etherType_0(bit<16>)| : 0}; with 2560 .. 2730;
  issue-2123.p4  # Cannot match {|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.etherType_0(bit<16>)| : 0, (|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.srcAddr_0(bit<48>)| : 0)[7:0], (|packet_extract_hdr.ethernet_0(bool)| ? |packet_extract_hdr.ethernet_hdr.ethernet.dstAddr_0(bit<48>)| : 0)[7:0]}; with 8 .. 16;
  issue1304.p4  # Cast failed: Pipeline<my_packet, my_metadata> with type Type_Specialized is not a Type_Declaration.
  invalid-hdr-warnings2.p4  # : No default value for type <Type_Unknown>(2) (Type_Unknown).
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unsupported assignment"
  control-hs-index-test5.p4  # Unsupported assignment rval h.h[tmp]; of type ArrayIndex
  checksum1-bmv2.p4  # Unsupported assignment rval pkt.lookahead<IPv4_up_to_ihl_only_h>(); of type MethodCallExpression
  checksum-l4-bmv2.p4  # Unsupported assignment rval pkt.lookahead<IPv4_up_to_ihl_only_h>(); of type MethodCallExpression
  issue1025-bmv2.p4  # Unsupported assignment rval pkt.lookahead<IPv4_up_to_ihl_only_h>(); of type MethodCallExpression
  invalid-hdr-warnings6.p4  # Unsupported assignment rval u_0/u[i_0/i]; of type ArrayIndex
  crc32-bmv2.p4  # Unsupported assignment rval packet.lookahead<p4calc_t>(); of type MethodCallExpression
  issue1560-bmv2.p4  # Unsupported assignment rval pkt.lookahead<IPv4_up_to_ihl_only_h>(); of type MethodCallExpression
  issue1765-bmv2.p4  # Unsupported assignment rval pkt.lookahead<IPv4_up_to_ihl_only_h>(); of type MethodCallExpression
  issue1989-bmv2.p4  # Unsupported assignment rval hdr.ethernet_stack[meta.color]; of type ArrayIndex
  issue2465-bmv2.p4  # Unsupported assignment rval b.lookahead<h>(); of type MethodCallExpression
  issue355-bmv2.p4  # Unsupported assignment rval pkt.lookahead<ethernet_t>(); of type MethodCallExpression
  issue3702-bmv2.p4  # Unsupported assignment rval pkt.lookahead<ipv4_t>(); of type MethodCallExpression
    up4.p4  # Unsupported assignment rval packet.lookahead<gtpu_t>(); of type MethodCallExpression
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unknown or unimplemented extern method: .*"
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
  hash-bmv2.p4  # Unknown or unimplemented extern method: *method.hash
  hashing-non-tuple-bmv2.p4  # Unknown or unimplemented extern method: *method.hash
  issue1049-bmv2.p4  # Unknown or unimplemented extern method: *method.hash
  issue2657-bmv2.p4  # Unknown or unimplemented extern method: *method.hash
  issue430-bmv2.p4  # Unknown or unimplemented extern method: *method.hash
  issue584-1-bmv2.p4  # Unknown or unimplemented extern method: *method.hash
  issue1517-bmv2.p4  # Unknown or unimplemented extern method: *method.random
  issue1566-bmv2.p4  # Unknown or unimplemented extern method: stats.count
  issue1566.p4  # Unknown or unimplemented extern method: stats.count
  issue2844-enum.p4  # Unknown or unimplemented extern method: stats.count
  issue1755-1-bmv2.p4  # Unknown or unimplemented extern method: packet.advance
  issue1755-bmv2.p4  # Unknown or unimplemented extern method: packet.advance
  parser-unroll-issue3537-1.p4  # Unknown or unimplemented extern method: packet.advance
  issue1768-bmv2.p4  # Unknown or unimplemented extern method: packet.lookahead
  issue356-bmv2.p4  # Unknown or unimplemented extern method: pkt.lookahead
  fabric.p4  # Unknown or unimplemented extern method: packet.lookahead
  issue1882-bmv2.p4  # Unknown or unimplemented extern method: extr.increment
  issue1882-1-bmv2.p4  # Unknown or unimplemented extern method: extr.increment
  issue2201-bmv2.p4  # Unknown or unimplemented extern method: *method.log_msg
  issue3001-1.p4  # Unknown or unimplemented extern method: *method.log_msg
  issue2664-bmv2.p4  # Unknown or unimplemented extern method: ipv4_checksum.update
  issue3091.p4  # Unknown or unimplemented extern method: *method.fn_foo
  issue841.p4  # Unknown or unimplemented extern method: checksum.get
  issue1520-bmv2.p4  # Unknown or unimplemented extern method: r.read
  issue1814-1-bmv2.p4  # Unknown or unimplemented extern method: testRegister.read
  issue1814-bmv2.p4  # Unknown or unimplemented extern method: testRegister.read
  issue1958.p4  # Unknown or unimplemented extern method: reg1.read
  named_meter_1-bmv2.p4  # Unknown or unimplemented extern method: my_meter.read
  named_meter_bmv2.p4  # Unknown or unimplemented extern method: my_meter.read
  issue1097-2-bmv2.p4  # Unknown or unimplemented extern method: r.write
  issue907-bmv2.p4  # Unknown or unimplemented extern method: r.write
  register-serenum-bmv2.p4  # Unknown or unimplemented extern method: reg.write
  issue242.p4  # Unknown or unimplemented extern method: debug.write
  issue696-bmv2.p4  # Unknown or unimplemented extern method: debug.write
  simplify_slice.p4  # Unknown or unimplemented extern method: debug.write
  slice-def-use.p4  # Unknown or unimplemented extern method: debug.write
  issue1352-bmv2.p4  # Unknown or unimplemented extern method: *method.digest
  issue430-1-bmv2.p4  # Unknown or unimplemented extern method: *method.digest
  v1model-digest-custom-type.p4  # Unknown or unimplemented extern method: *method.digest
  v1model-digest-containing-ser-enum.p4  # Unknown or unimplemented extern method: *method.digest
  basic2-bmv2.p4  # Unknown or unimplemented extern method: *method.update_checksum
  issue134-bmv2.p4  # Unknown or unimplemented extern method: *method.update_checksum
  xor_test.p4  # Unknown or unimplemented extern method: *method.update_checksum
  subparser-with-header-stack-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  issue1824-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  issue561-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  verify_disjunction.p4  # Unknown or unimplemented extern method: *method.verify
  verify-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  dash-pipeline.p4  # Unknown or unimplemented extern method: *method.verify
  checksum2-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  checksum3-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  header-stack-ops-bmv2.p4  # Unknown or unimplemented extern method: *method.verify
  issue1079-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue1739-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue249.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  basic_routing-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  flowlet_switching-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue1630-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue270-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue298-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue461-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue655-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  issue655.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  p416-type-use3.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  v1model-special-ops-bmv2.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  pins_fabric.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  pins_wbb.p4  # Unknown or unimplemented extern method: *method.verify_checksum
  pins_middleblock.p4  # Unknown or unimplemented extern method: *method.verify_checksum
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Unable to find var .* in the symbolic environment"
  array-copy-bmv2.p4  # Unable to find var h.h1; in the symbolic environment.
  gauntlet_hdr_set_valid-bmv2.p4  # Unable to find var local_h_0/local_h; in the symbolic environment.
  issue1210.p4  # Unable to find var meta.foo; in the symbolic environment.
  issue1409-bmv2.p4  # Unable to find var headers.test.lastIndex; in the symbolic environment.
  issue1955.p4  # Unable to find var p1_ipv4_ethertypes/ipv4_ethertypes; in the symbolic environment.
  issue232-bmv2.p4  # Unable to find var inKey_0/inKey; in the symbolic environment.
  issue1607-bmv2.p4  # Unable to find var hdr.stack.last.*valid; in the symbolic environment.
  issue3374.p4  # Unable to find var hdrs.vlan_tag.last.ether_type; in the symbolic environment.
  parser-unroll-test2.p4  # Unable to find var hdr.srcRoutes.last.bos; in the symbolic environment.
  parser-unroll-test3.p4  # Unable to find var hdr.srcRoutes.last.bos; in the symbolic environment.
  parser-unroll-test9.p4  # Unable to find var hdr.h.last.i2; in the symbolic environment.
  stack_complex-bmv2.p4  # Unable to find var h.hs.last.f2; in the symbolic environment.
  ternary2-bmv2.p4  # Unable to find var hdrs.extra.last.b2; in the symbolic environment.
  parser-unroll-test6.p4  # Unable to find var headers.test.lastIndex; in the symbolic environment.
  pvs-bitstring-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-nested-struct.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-1-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-2-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  pvs-struct-3-bmv2.p4  # Unable to find var pvs_0/pvs; in the symbolic environment.
  union-valid-bmv2.p4  # Unable to find var h.u.*valid; in the symbolic environment.
  v1model-p4runtime-enumint-types1.p4  # Unable to find var valueset1_0/valueset1; in the symbolic environment.
  v1model-p4runtime-most-types1.p4  # Unable to find var valueset1_0/valueset1; in the symbolic environment.
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "of type ArrayIndex is not a valid StateVariable."
  control-hs-index-test1.p4
  control-hs-index-test2.p4
  control-hs-index-test3.p4
  control-hs-index-test4.p4
  control-hs-index-test6.p4
  gauntlet_index_1-bmv2.p4
  gauntlet_index_5-bmv2.p4
  gauntlet_index_7-bmv2.p4
  gauntlet_index_8-bmv2.p4
  predication_issue_2.p4
  predication_issue_3.p4
  predication_issue_4.p4
  issue2726-bmv2.p4
  runtime-index-bmv2.p4
  runtime-index-2-bmv2.p4
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "The list of target fields and the list of source fields have different sizes."
  issue2488-bmv2.p4
)

p4tools_add_xfail_reason(
  "flay-p4c-bmv2-v1model"
  "Parser state .* was already visited. We currently do not support parser loops."
  issue2314.p4
  invalid-hdr-warnings1.p4
  invalid-hdr-warnings4.p4
  issue692-bmv2.p4
  parser-unroll-issue3537.p4
  issue1897-bmv2.p4
  issue1937-2-bmv2.p4
  issue281.p4
)
