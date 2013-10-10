//
// Copyright 2011-2012 Ettus Research LLC
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

// The packet padder 36 for use with slave fifo32
// Packet padder understands the concept of LUTs,
// and will forward packets through the interface,
// adding zero padding as needed to properly flush.
// The padder will never write a packet across a LUT boundary.
// When flushing, padder writes out zeros until the LUT boundary.
// Requires that the input line0 be a VITA header, and SOF set.
// Flush when the LUT is partially filled and timeout is reached,
// or when the LUT is partially filled and the DSP is inactive.

module packet_padder36
#(
    parameter RX_IDLE_FLUSH_CYCLES = 65536, //about 1ms at 64MHz clock
    parameter MAX_LUT_LINES32 = 4096 //how many 32bit lines in a LUT
)
(
    input clk, input reset,
    input [35:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,
    output [35:0] data_o,
    output src_rdy_o,
    input dst_rdy_i,
    input rx_dsp_active
);

    //state machine definitions
    localparam STATE_READ_HDR = 0;
    localparam STATE_WRITE_HDR = 1;
    localparam STATE_FORWARD = 2;
    localparam STATE_WRITE_PAD = 3;
    reg [1:0] state;

    //keep track of the outgoing lines
    reg [15:0] line_count;
    wire line_count_done = line_count == 1;
    wire lut_is_empty = line_count == MAX_LUT_LINES32;
    always @(posedge clk) begin
        if (reset) begin
            line_count <= MAX_LUT_LINES32;
        end
        else if (src_rdy_o && dst_rdy_i) begin
            line_count <= (line_count_done)? MAX_LUT_LINES32 : line_count - 1;
        end
    end

    //count the number of cycles since RX data so we can force a flush
    reg [17:0] non_rx_cycles;
    wire idle_timeout = (non_rx_cycles == RX_IDLE_FLUSH_CYCLES);
    always @(posedge clk) begin
        if(reset || state != STATE_READ_HDR) begin
            non_rx_cycles <= 0;
        end
        else if (~idle_timeout) begin
            non_rx_cycles <= non_rx_cycles + 1;
        end
    end

    //flush when we have written data to a LUT and either idle or non active DSP
    wire force_flush = ~lut_is_empty && (idle_timeout || ~rx_dsp_active);

    //the padding state machine
    reg [31:0] vita_hdr;
    reg has_vita_hdr;
    always @(posedge clk) begin
        if (reset) begin
            state <= STATE_READ_HDR;
        end
        else case(state)

        STATE_READ_HDR: begin
            if (src_rdy_i && dst_rdy_o && data_i[32]) begin
                vita_hdr <= data_i[31:0];
                has_vita_hdr <= 1;
                state <= (data_i[15:0] > line_count)? state <= STATE_WRITE_PAD : STATE_WRITE_HDR;
            end
            else if (force_flush) begin
                has_vita_hdr <= 0;
                state <= STATE_WRITE_PAD;
            end
        end

        STATE_WRITE_HDR: begin
            if (src_rdy_o && dst_rdy_i) begin
                state <= STATE_FORWARD;
            end
        end

        STATE_FORWARD: begin
            if (src_rdy_i && dst_rdy_o && data_i[33]) begin
                state <= STATE_READ_HDR;
            end
        end

        STATE_WRITE_PAD: begin
            if (src_rdy_o && dst_rdy_i && line_count_done) begin
                state <= (has_vita_hdr)? STATE_WRITE_HDR : STATE_READ_HDR;
            end
        end

        endcase //state
    end

    //assign outgoing signals
    assign dst_rdy_o = (state == STATE_READ_HDR)? 1 : ((state == STATE_FORWARD)? dst_rdy_i : 0);
    assign src_rdy_o = (state == STATE_WRITE_HDR || state == STATE_WRITE_PAD)? 1 : ((state == STATE_FORWARD )? src_rdy_i : 0);
    assign data_o = (state == STATE_WRITE_HDR)? {4'b0001, vita_hdr} : ((state == STATE_FORWARD)? data_i : 0);

endmodule // packet_padder36




