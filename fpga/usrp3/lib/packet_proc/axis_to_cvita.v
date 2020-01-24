//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`default_nettype none

module axis_to_cvita
(
        input wire           clk,

	input wire [63:0]    s_axis_tdata,
        input wire           s_axis_tlast,
        input wire           s_axis_tvalid,
        output wire          s_axis_tready,

	output wire [63:0]   o_tdata,
        output wire          o_tlast,
        output wire          o_tvalid,
        input wire           o_tready
);

	assign s_axis_tready = o_tready;

	assign o_tdata = {s_axis_tdata[31:0], s_axis_tdata[63:32]};
	assign o_tlast = s_axis_tlast;
	assign o_tvalid = s_axis_tvalid;

endmodule // axis_to_cvita

`default_nettype wire

