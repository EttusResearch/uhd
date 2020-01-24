//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Map the endpoint dest part of the SID in the CVITA header to a destination
// This destination (o_tdest) signal will be valid with o_tdata
// This only works with VALID CVITA frames

module cvita_dest_lookup_legacy
#(
    parameter DEST_WIDTH = 4
)
(
    input clk, input rst,
    input set_stb, input [7:0] set_addr, input [DEST_WIDTH-1:0] set_data,
    input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready,
    output [DEST_WIDTH-1:0] o_tdest
);

    reg [7:0] endpoint;
    ram_2port #(.DWIDTH(DEST_WIDTH), .AWIDTH(8)) dest_lut
    (
        .clka(clk), .ena(1'b1), .wea(set_stb), .addra(set_addr), .dia(set_data), .doa(),
        .clkb(clk), .enb(1'b1), .web(1'b0), .addrb(endpoint), .dib(8'hff), .dob(o_tdest)
    );

    reg forward;
    reg [1:0] count;
    always @(posedge clk) begin
        if (rst) begin
            forward <= 1'b0;
            count <= 2'b0;
        end
        else if (forward == 1'b0 && i_tvalid) begin
            if (count == 2'b11) forward <= 1'b1;
            endpoint <= i_tdata[23:16];
            count <= count + 1'b1;
        end
        else if (forward == 1'b1 && i_tvalid && i_tready && i_tlast) begin
            forward <= 1'b0;
            count <= 2'b0;
        end
    end

    assign o_tdata = i_tdata;
    assign o_tlast = i_tlast;
    assign o_tvalid = i_tvalid && forward;
    assign i_tready = o_tready && forward;

endmodule // cvita_dest_lookup_legacy
