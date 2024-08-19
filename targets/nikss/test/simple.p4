#include <core.p4>
#include <psa.p4>

typedef bit<48> ethernet_addr_t;

struct empty_metadata_t {
}

header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    bit<16>         ether_type;
}
struct headers_t {
    ethernet_t ethernet;
}

struct local_metadata_t {
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_parser_input_metadata_t standard_metadata, in empty_metadata_t resub_meta, in empty_metadata_t recirc_meta) {
    state start {
        packet.extract(headers.ethernet);
        transition accept;
    }
}

control packet_deparser(packet_out packet, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit(headers);
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, in psa_ingress_input_metadata_t standard_metadata,
                inout psa_ingress_output_metadata_t ostd) {

    action forward(PortId_t output_port) {
        send_to_port(ostd, output_port);
    }

    action forward_to_one() {
        send_to_port(ostd, (PortId_t) PORT1);
    }

    table tbl_switching {
        key = {
            headers.ethernet.dst_addr : exact;
        }

        actions = {
            forward;
            forward_to_one;
        }
        default_action = forward_to_one;
    }

    apply {
        tbl_switching.apply();
    }

}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

parser egress_parser(packet_in buffer, out headers_t headers, inout local_metadata_t local_metadata, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }

}

control egress_deparser(packet_out packet, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t headers, in local_metadata_t local_metadata, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        packet.emit(headers);
    }
}


IngressPipeline(packet_parser(), ingress(), packet_deparser()) ip;

EgressPipeline(egress_parser(), egress(), egress_deparser()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
