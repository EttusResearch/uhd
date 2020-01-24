//
// Copyright 2016 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include <complex>
#include "ap_int.h"

struct axis_cplx {
    std::complex<short int> data;
    ap_uint<1> last;
};

// AXI-Stream port type is compatible with pointer, reference, & array input / ouputs only
// See UG902 Vivado High Level Synthesis guide (2014.4) pg 157 Figure 1-49
void addsub_hls (axis_cplx &a, axis_cplx &b, axis_cplx &add, axis_cplx &sub) {

    // Remove ap ctrl ports (ap_start, ap_ready, ap_idle, etc) since we only use the AXI-Stream ports
    #pragma HLS INTERFACE ap_ctrl_none port=return
    // Set ports as AXI-Stream
    #pragma HLS INTERFACE axis port=sub
    #pragma HLS INTERFACE axis port=add
    #pragma HLS INTERFACE axis port=a
    #pragma HLS INTERFACE axis port=b
    // Need to pack our complex<short int> into a 32-bit word
    // Otherwise, compiler complains that our AXI-Stream interfaces have two data fields (i.e. data.real, data.imag)
    #pragma HLS DATA_PACK variable=sub.data
    #pragma HLS DATA_PACK variable=add.data
    #pragma HLS DATA_PACK variable=a.data
    #pragma HLS DATA_PACK variable=b.data

    // Complex add / subtract
    add.data.real() = a.data.real() + b.data.real();
    add.data.imag() = a.data.imag() + b.data.imag();
    sub.data.real() = a.data.real() - b.data.real();
    sub.data.imag() = a.data.imag() - b.data.imag();
    // Pass through tlast
    add.last = a.last;
    sub.last = a.last;
}
