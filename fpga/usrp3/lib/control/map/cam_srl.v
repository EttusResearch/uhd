/*

Copyright (c) 2015-2016 Alex Forencich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// Language: Verilog 2001

`timescale 1ns / 1ps

/*
 * Content Addressable Memory (shift register based)
 */
module cam_srl #(
    // search data bus width
    parameter DATA_WIDTH = 64,
    // memory size in log2(words)
    parameter ADDR_WIDTH = 5,
    // width of data bus slices (4 for SRL16, 5 for SRL32)
    parameter SLICE_WIDTH = 4
)
(
    input  wire                     clk,
    input  wire                     rst,

    input  wire [ADDR_WIDTH-1:0]    write_addr,
    input  wire [DATA_WIDTH-1:0]    write_data,
    input  wire                     write_delete,
    input  wire                     write_enable,
    output wire                     write_busy,

    input  wire [DATA_WIDTH-1:0]    compare_data,
    output wire [2**ADDR_WIDTH-1:0] match_many,
    output wire [2**ADDR_WIDTH-1:0] match_single,
    output wire [ADDR_WIDTH-1:0]    match_addr,
    output wire                     match
);

// total number of slices (enough to cover DATA_WIDTH with address inputs)
localparam SLICE_COUNT = (DATA_WIDTH + SLICE_WIDTH - 1) / SLICE_WIDTH;
// depth of RAMs
localparam RAM_DEPTH = 2**ADDR_WIDTH;

localparam [1:0]
    STATE_INIT = 2'd0,
    STATE_IDLE = 2'd1,
    STATE_WRITE = 2'd2,
    STATE_DELETE = 2'd3;

reg [1:0] state_reg = STATE_INIT, state_next;

wire [SLICE_COUNT*SLICE_WIDTH-1:0] compare_data_padded = {{SLICE_COUNT*SLICE_WIDTH-DATA_WIDTH{1'b0}}, compare_data};
wire [SLICE_COUNT*SLICE_WIDTH-1:0] write_data_padded = {{SLICE_COUNT*SLICE_WIDTH-DATA_WIDTH{1'b0}}, write_data};

reg [SLICE_WIDTH-1:0] count_reg = {SLICE_WIDTH{1'b1}}, count_next;

reg [SLICE_COUNT-1:0] shift_data;
reg [RAM_DEPTH-1:0] shift_en;

reg [ADDR_WIDTH-1:0] write_addr_reg = {ADDR_WIDTH{1'b0}}, write_addr_next;
reg [SLICE_COUNT*SLICE_WIDTH-1:0] write_data_padded_reg = {SLICE_COUNT*SLICE_WIDTH{1'b0}}, write_data_padded_next;

reg write_busy_reg = 1'b1;

assign write_busy = write_busy_reg;

reg [RAM_DEPTH-1:0] match_raw_out[SLICE_COUNT-1:0];
reg [RAM_DEPTH-1:0] match_many_raw;
reg [RAM_DEPTH-1:0] match_many_reg = {RAM_DEPTH{1'b0}};

assign match_many = match_many_reg;

integer k;

always @* begin
    match_many_raw = ~shift_en;
    for (k = 0; k < SLICE_COUNT; k = k + 1) begin
        match_many_raw = match_many_raw & match_raw_out[k];
    end
end

cam_priority_encoder #(
    .WIDTH(RAM_DEPTH),
    .LSB_PRIORITY("HIGH")
)
priority_encoder_inst (
    .input_unencoded(match_many_reg),
    .output_valid(match),
    .output_encoded(match_addr),
    .output_unencoded(match_single)
);

integer i;

// SRLs
genvar row_ind, slice_ind;
generate
    for (row_ind = 0; row_ind < RAM_DEPTH; row_ind = row_ind + 1) begin : row
        for (slice_ind = 0; slice_ind < SLICE_COUNT; slice_ind = slice_ind + 1) begin : slice
            reg [2**SLICE_WIDTH-1:0] srl_mem = {2**SLICE_WIDTH{1'b0}};

            // match
            always @* begin
                match_raw_out[slice_ind][row_ind] = srl_mem[compare_data_padded[SLICE_WIDTH * slice_ind +: SLICE_WIDTH]];
            end

            // write
            always @(posedge clk) begin
                if (shift_en[row_ind]) begin
                    srl_mem <= {srl_mem[2**SLICE_WIDTH-2:0], shift_data[slice_ind]};
                end
            end
        end
    end
endgenerate

// match
always @(posedge clk) begin
    match_many_reg <= match_many_raw;
end

// write
always @* begin
    state_next = STATE_IDLE;

    count_next = count_reg;
    shift_data = {SLICE_COUNT{1'b0}};
    shift_en = {RAM_DEPTH{1'b0}};

    write_addr_next = write_addr_reg;
    write_data_padded_next = write_data_padded_reg;

    case (state_reg)
        STATE_INIT: begin
            // zero out shift registers
            shift_en = {RAM_DEPTH{1'b1}};
            shift_data = {SLICE_COUNT{1'b0}};

            if (count_reg == 0) begin
                state_next = STATE_IDLE;
            end else begin
                count_next = count_reg - 1;
                state_next = STATE_INIT;
            end
        end
        STATE_IDLE: begin
            if (write_enable) begin
                write_addr_next = write_addr;
                write_data_padded_next = write_data_padded;
                count_next = {SLICE_WIDTH{1'b1}};
                if (write_delete) begin
                    state_next = STATE_DELETE;
                end else begin
                    state_next = STATE_WRITE;
                end
            end else begin
                state_next = STATE_IDLE;
            end
        end
        STATE_WRITE: begin
            // write entry
            shift_en = 1'b1 << write_addr;

            for (i = 0; i < SLICE_COUNT; i = i + 1) begin
                shift_data[i] = count_reg == write_data_padded_reg[SLICE_WIDTH * i +: SLICE_WIDTH];
            end

            if (count_reg == 0) begin
                state_next = STATE_IDLE;
            end else begin
                count_next = count_reg - 1;
                state_next = STATE_WRITE;
            end
        end
        STATE_DELETE: begin
            // delete entry
            shift_en = 1'b1 << write_addr;
            shift_data = {SLICE_COUNT{1'b0}};

            if (count_reg == 0) begin
                state_next = STATE_IDLE;
            end else begin
                count_next = count_reg - 1;
                state_next = STATE_DELETE;
            end
        end
    endcase
end

always @(posedge clk) begin
    if (rst) begin
        state_reg <= STATE_INIT;
        count_reg <= {SLICE_WIDTH{1'b1}};
        write_busy_reg <= 1'b1;
    end else begin
        state_reg <= state_next;
        count_reg <= count_next;
        write_busy_reg <= state_next != STATE_IDLE;
    end

    write_addr_reg <= write_addr_next;
    write_data_padded_reg <= write_data_padded_next;
end

endmodule
