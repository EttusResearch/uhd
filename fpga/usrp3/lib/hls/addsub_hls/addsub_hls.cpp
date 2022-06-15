//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <complex>

typedef std::complex<short int> axis_data;
typedef hls::axis<axis_data, 0, 0, 0> axis_cplx;

// The AXI-Stream port type is compatible with pointer, reference, array, and
// stream input/outputs only. See UG902 Vivado High Level Synthesis guide for
// details.
void addsub_hls(
    hls::stream<axis_cplx>& a,
    hls::stream<axis_cplx>& b,
    hls::stream<axis_cplx>& add,
    hls::stream<axis_cplx>& sub)
{
    // Remove ap ctrl ports (ap_start, ap_ready, ap_idle, etc) since we only
    // use the AXI-Stream ports.
    #pragma HLS INTERFACE ap_ctrl_none port = return

    // Set ports as AXI-Stream
    #pragma HLS INTERFACE axis port = sub
    #pragma HLS INTERFACE axis port = add
    #pragma HLS INTERFACE axis port = a
    #pragma HLS INTERFACE axis port = b

    // Pipeline with initiation interval of 1 clock cycle
    #pragma HLS PIPELINE II=1

    // Read the next pair of AXI-stream inputs
    axis_cplx a_trans, b_trans;
    a.read(a_trans);
    b.read(b_trans);

    // Complex addition
    axis_cplx add_trans;
    add_trans.data = a_trans.data + b_trans.data;
    add_trans.last = a_trans.last;
    add.write(add_trans);

    // Complex subtraction
    axis_cplx sub_trans;
    sub_trans.data = a_trans.data - b_trans.data;
    sub_trans.last = a_trans.last;
    sub.write(sub_trans);
}
