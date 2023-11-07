#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct local_metadata_t {
}

struct Headers {
    ethernet_t ethernet;
}

parser p(packet_in pkt, out Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.ethernet);
        transition accept;
    }
}

control vrfy(inout Headers h, inout local_metadata_t local_metadata) {
    apply { }
}

control ingress(inout Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t s) {
    apply {
    }
}

control egress(inout Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t s) {
    apply {
        if (s.instance_type == 2) {
            h.ethernet.src_addr = 2;
        }
        if (s.instance_type == 0) {
            clone_preserving_field_list(CloneType.I2E, 1, 8w1);
        }
     }
}

control update(inout Headers h, inout local_metadata_t local_metadata) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}


V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
