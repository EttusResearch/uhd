//
// Copyright 2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

//demux an input stream based on the SID
//output packet has SID removed from header

module vita_packet_demux36
#(
    parameter NUMCHAN = 1,
    parameter SID_BASE = 0
)
(
    input clk, input rst,

    input [35:0] in_data,
    input in_src_rdy,
    output in_dst_rdy,

    output [35:0] out_data,
    output [NUMCHAN-1:0] out_src_rdy,
    input [NUMCHAN-1:0] out_dst_rdy
);

    reg [1:0] state;
    localparam STATE_WAIT_HDR = 0;
    localparam STATE_PROC_SID = 1;
    localparam STATE_WRITE_HDR = 2;
    localparam STATE_FORWARD = 3;

    reg [31:0] hdr;
    reg [NUMCHAN-1:0] sid;
    wire has_sid = in_data[28];
    reg has_sid_reg;

    wire my_out_dst_rdy = out_dst_rdy[sid];
    wire my_out_src_rdy = out_src_rdy[sid];

    always @(posedge clk) begin
        if (rst) begin
            state <= STATE_WAIT_HDR;
        end
        else case(state)

        STATE_WAIT_HDR: begin
            if (in_src_rdy && in_dst_rdy && in_data[32]) begin
                state <= (has_sid)? STATE_PROC_SID : STATE_WRITE_HDR;
            end
            sid <= 0;
            hdr <= in_data[31:0];
            has_sid_reg <= has_sid;
        end

        STATE_PROC_SID: begin
            if (in_src_rdy && in_dst_rdy) begin
                state <= STATE_WRITE_HDR;
                sid <= in_data[31:0] - SID_BASE;
                hdr[28] <= 1'b0; //clear has sid
                hdr[15:0] <= hdr[15:0] - 1'b1; //subtract a line
            end
        end

        STATE_WRITE_HDR: begin
            if (my_out_src_rdy && my_out_dst_rdy) begin
                state <= STATE_FORWARD;
            end
        end

        STATE_FORWARD: begin
            if (my_out_src_rdy && my_out_dst_rdy && out_data[33]) begin
                state <= STATE_WAIT_HDR;
            end
        end

        endcase //state
    end

    assign out_data = (state == STATE_WRITE_HDR)? {4'b0001, hdr} : in_data;
    wire out_src_rdy_i = (state == STATE_WRITE_HDR)? 1'b1 : ((state == STATE_FORWARD)? in_src_rdy : 1'b0);
    assign in_dst_rdy = (state == STATE_WAIT_HDR || state == STATE_PROC_SID)? 1'b1 : ((state == STATE_FORWARD)? my_out_dst_rdy : 1'b0);

    genvar i;
    generate
    for(i = 0; i < NUMCHAN; i = i + 1) begin:valid_assign
        assign out_src_rdy[i] = (i == sid)? out_src_rdy_i : 1'b0;
    end
    endgenerate

endmodule //vita_packet_demux36
