//
// Copyright 2022 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include <complex>

typedef std::complex<short int> axis_data;
typedef hls::axis<axis_data, 0, 0, 0> trans_pkt;

// AXI-Stream port type is compatible with pointer, reference, & array input / ouputs only
// See UG902 Vivado High Level Synthesis guide (2014.4) pg 157 Figure 1-49
void addsub_hls(hls::stream<trans_pkt>& a,
    hls::stream<trans_pkt>& b,
    hls::stream<trans_pkt>& add,
    hls::stream<trans_pkt>& sub)
{
	// Remove ap ctrl ports (ap_start, ap_ready, ap_idle, etc) since we only use the
	// AXI-Stream ports
	#pragma HLS INTERFACE ap_ctrl_none port = return

	// Set ports as AXI-Stream
	#pragma HLS INTERFACE axis port = sub
	#pragma HLS INTERFACE axis port = add
	#pragma HLS INTERFACE axis port = a
	#pragma HLS INTERFACE axis port = b

    // Complex add / subtract
    trans_pkt a_pkt;
    a.read(a_pkt);

    trans_pkt b_pkt;
    b.read(b_pkt);

    trans_pkt add_pkt;
    add_pkt.data = {
        a_pkt.data.real() + b_pkt.data.real(),
		a_pkt.data.imag() + b_pkt.data.imag()};
    add_pkt.last = a_pkt.last;
    add.write(add_pkt);

    trans_pkt sub_pkt;
    sub_pkt.data = {
        a_pkt.data.real() - b_pkt.data.real(),
		a_pkt.data.imag() - b_pkt.data.imag()};
    sub_pkt.last = a_pkt.last;
    sub.write(sub_pkt);
}
