#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<16>     total_len;
    bit<16>     identification;
    bit<1>      reserved;
    bit<1>      do_not_fragment;
    bit<1>      more_fragments;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     header_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

struct local_metadata_t {
}

struct Headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    udp_t      udp;
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
    bit<16> value = 16w0;

    action check() {
        value = 16w1;
    }

    table toggle_check {
        key = {
            h.ethernet.dst_addr : ternary @name("dst_eth");
        }
        actions = {
            check();
            NoAction();
        }
        const default_action = check();
    }


    apply {
        toggle_check.apply();
        h.ethernet.ether_type = value;
    }

}

control egress(inout Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t s) {
    apply { }
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
