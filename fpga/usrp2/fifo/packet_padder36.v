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

// The packet padder 36 for use with RX VITA stream output.
// Packet padder understands the concept of USB LUTs,
// and will forward packets through the interface,
// adding zero padding as needed to properly flush.
// The padder will never write a packet across a LUT boundary.
// When flushing, padder writes out zeros until the LUT boundary.
// Requires that the input line0 be a VITA header, and SOF set.
// Flush when the LUT is partially filled and timeout is reached,
// or when the LUT is partially filled and the DSP is inactive.

module packet_padder36
#(
    parameter BASE = 0,

    //default is 16K LUT
    parameter DEFAULT_LINES32 = 4096,

    //default about 1ms at 64MHz clock
    parameter DEFAULT_IDLE_CYC = 65536
)
(
    input clk, input reset,

    //setting bus
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    //input interface
    input [35:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,

    //output interface
    output [35:0] data_o,
    output src_rdy_o,
    input dst_rdy_i,

    input always_flush
);

    wire lut_lines_changed;
    wire [15:0] max_lut_lines32;
    setting_reg #(.my_addr(BASE+0),.width(16),.at_reset(DEFAULT_LINES32)) sr_num_lines(
        .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(max_lut_lines32),.changed(lut_lines_changed));

    wire idle_cyc_changed;
    wire [17:0] idle_flush_cycles;
    setting_reg #(.my_addr(BASE+1),.width(18),.at_reset(DEFAULT_IDLE_CYC)) sr_flush_cyc(
        .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
        .out(idle_flush_cycles),.changed(idle_cyc_changed));

    //state machine definitions
    localparam STATE_READ_HDR = 0;
    localparam STATE_WRITE_HDR = 1;
    localparam STATE_FORWARD = 2;
    localparam STATE_WRITE_PAD = 3;
    reg [1:0] state;

    //keep track of the outgoing lines
    reg [15:0] line_count;
    wire line_count_done = line_count == 1;
    wire lut_is_empty = line_count == max_lut_lines32;
    always @(posedge clk) begin
        if (reset || lut_lines_changed) begin
            line_count <= max_lut_lines32;
        end
        else if (src_rdy_o && dst_rdy_i) begin
            line_count <= (line_count_done)? max_lut_lines32 : line_count - 1;
        end
    end

    //count the number of cycles since RX data so we can force a flush
    reg [17:0] non_rx_cycles;
    wire idle_timeout = (non_rx_cycles == idle_flush_cycles);
    always @(posedge clk) begin
        if(reset || state != STATE_READ_HDR || idle_cyc_changed) begin
            non_rx_cycles <= 0;
        end
        else if (~idle_timeout) begin
            non_rx_cycles <= non_rx_cycles + 1;
        end
    end

    //flush when we have written data to a LUT and either idle or non active DSP
    wire force_flush = ~lut_is_empty && (idle_timeout || always_flush);

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
                state <= (data_i[15:0] > line_count)? STATE_WRITE_PAD : STATE_WRITE_HDR;
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




