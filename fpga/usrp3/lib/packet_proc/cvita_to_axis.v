//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`default_nettype none

module cvita_to_axis
(
	input wire           clk,

	input wire [63:0]    i_tdata,
        input wire           i_tlast,
        input wire           i_tvalid,
        output wire          i_tready,

	output wire [63:0]   m_axis_tdata,
        output wire          m_axis_tlast,
        output wire          m_axis_tvalid,
        input wire           m_axis_tready
);

	assign i_tready = m_axis_tready;

	assign m_axis_tdata = {i_tdata[31:0], i_tdata[63:32]};
	assign m_axis_tlast = i_tlast;
	assign m_axis_tvalid = i_tvalid;

endmodule // cvita_to_axis

`default_nettype wire

