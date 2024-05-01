////////////////////////////////////////////
// THIS FILE IS A MOCK. IT DOES NOT REPRESENT CONTENT OF THE REAL xsa.p4 AND ONLY CONTAINS
// SIGNATURES TO ENABLE COMPILATION OF CUSTOM XSA P4 PROGRAMS. THE SIGNATURES ARE DERIVED
// FROM v1model.p4 and the publicly available programs at https://github.com/esnet/esnet-smartnic-hw/.
///

#ifndef _XILINX_XSA_
#define _XILINX_XSA_

#include <core.p4>

struct standard_metadata_t {
    bit<1>  drop;
    error   parser_error;
}

match_kind {
    range,
}

extern UserExtern<T, I> {
    UserExtern(bit<16> val);
    void apply(in T extern_in, out I extern_out);
}

parser Parser<H, M>(packet_in b,
                    out H hdr,
                    inout M meta,
                    inout standard_metadata_t standard_metadata);

control MatchAction<H, M>(inout H hdr,
                          inout M meta,
                          inout standard_metadata_t standard_metadata);

control Deparser<H, M>(packet_out b,
                       in H hdr,
                       inout M meta,
                       inout standard_metadata_t standard_metadata);

package XilinxPipeline<H, M>(Parser<H, M> p,
                             MatchAction<H, M> ma,
                             Deparser<H, M> dep);


#endif /* _XILINX_XSA_ */
