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
 * Content Addressable Memory (block RAM based)
 */
module cam_bram #(
    // search data bus width
    parameter DATA_WIDTH = 64,
    // memory size in log2(words)
    parameter ADDR_WIDTH = 5,
    // width of data bus slices
    parameter SLICE_WIDTH = 9
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

localparam [2:0]
    STATE_INIT = 3'd0,
    STATE_IDLE = 3'd1,
    STATE_DELETE_1 = 3'd2,
    STATE_DELETE_2 = 3'd3,
    STATE_WRITE_1 = 3'd4,
    STATE_WRITE_2 = 3'd5;

reg [2:0] state_reg = STATE_INIT, state_next;

wire [SLICE_COUNT*SLICE_WIDTH-1:0] compare_data_padded = {{SLICE_COUNT*SLICE_WIDTH-DATA_WIDTH{1'b0}}, compare_data};
wire [SLICE_COUNT*SLICE_WIDTH-1:0] write_data_padded = {{SLICE_COUNT*SLICE_WIDTH-DATA_WIDTH{1'b0}}, write_data};

reg [SLICE_WIDTH-1:0] count_reg = {SLICE_WIDTH{1'b1}}, count_next;

reg [SLICE_COUNT*SLICE_WIDTH-1:0] ram_addr = {SLICE_COUNT*SLICE_WIDTH{1'b0}};
reg [RAM_DEPTH-1:0] set_bit;
reg [RAM_DEPTH-1:0] clear_bit;
reg wr_en;

reg [ADDR_WIDTH-1:0] write_addr_reg = {ADDR_WIDTH{1'b0}}, write_addr_next;
reg [SLICE_COUNT*SLICE_WIDTH-1:0] write_data_padded_reg = {SLICE_COUNT*SLICE_WIDTH{1'b0}}, write_data_padded_next;
reg write_delete_reg = 1'b0, write_delete_next;

reg write_busy_reg = 1'b1;

assign write_busy = write_busy_reg;

reg [RAM_DEPTH-1:0] match_raw_out[SLICE_COUNT-1:0];
reg [RAM_DEPTH-1:0] match_many_raw;

assign match_many = match_many_raw;

reg [DATA_WIDTH-1:0] erase_ram [RAM_DEPTH-1:0];
reg [DATA_WIDTH-1:0] erase_data = {DATA_WIDTH{1'b0}};
reg erase_ram_wr_en;

integer i;

initial begin
    for (i = 0; i < RAM_DEPTH; i = i + 1) begin
        erase_ram[i] = {SLICE_COUNT*SLICE_WIDTH{1'b0}};
    end
end

integer k;

always @* begin
    match_many_raw = {RAM_DEPTH{1'b1}};
    for (k = 0; k < SLICE_COUNT; k = k + 1) begin
        match_many_raw = match_many_raw & match_raw_out[k];
    end
end

cam_priority_encoder #(
    .WIDTH(RAM_DEPTH),
    .LSB_PRIORITY("HIGH")
)
priority_encoder_inst (
    .input_unencoded(match_many_raw),
    .output_valid(match),
    .output_encoded(match_addr),
    .output_unencoded(match_single)
);

// BRAMs
genvar slice_ind;
generate
    for (slice_ind = 0; slice_ind < SLICE_COUNT; slice_ind = slice_ind + 1) begin : slice
        localparam W = slice_ind == SLICE_COUNT-1 ? DATA_WIDTH-SLICE_WIDTH*slice_ind : SLICE_WIDTH;

        wire [RAM_DEPTH-1:0] match_data;
        wire [RAM_DEPTH-1:0] ram_data;

        ram_2port #(
            .DWIDTH(RAM_DEPTH),
            .AWIDTH(W)
        )
        ram_inst
        (
            .clka(clk),
            .ena(1'b1),
            .wea(1'b0),
            .addra(compare_data[SLICE_WIDTH * slice_ind +: W]),
            .dia({RAM_DEPTH{1'b0}}),
            .doa(match_data),
            .clkb(clk),
            .enb(1'b1),
            .web(wr_en),
            .addrb(ram_addr[SLICE_WIDTH * slice_ind +: W]),
            .dib((ram_data & ~clear_bit) | set_bit),
            .dob(ram_data)
        );

        always @* begin
            match_raw_out[slice_ind] <= match_data;
        end
    end
endgenerate

// erase
always @(posedge clk) begin
    erase_data <= erase_ram[write_addr_next];
    if (erase_ram_wr_en) begin
        erase_data <= write_data_padded_reg;
        erase_ram[write_addr_next] <= write_data_padded_reg;
    end
end

// write
always @* begin
    state_next = STATE_IDLE;

    count_next = count_reg;
    ram_addr = erase_data;
    set_bit = {RAM_DEPTH{1'b0}};
    clear_bit = {RAM_DEPTH{1'b0}};
    wr_en = 1'b0;

    erase_ram_wr_en = 1'b0;

    write_addr_next = write_addr_reg;
    write_data_padded_next = write_data_padded_reg;
    write_delete_next = write_delete_reg;

    case (state_reg)
        STATE_INIT: begin
            // zero out RAMs
            ram_addr = {SLICE_COUNT{count_reg}} & {{SLICE_COUNT*SLICE_WIDTH-DATA_WIDTH{1'b0}}, {DATA_WIDTH{1'b1}}};
            set_bit = {RAM_DEPTH{1'b0}};
            clear_bit = {RAM_DEPTH{1'b1}};
            wr_en = 1'b1;

            if (count_reg == 0) begin
                state_next = STATE_IDLE;
            end else begin
                count_next = count_reg - 1;
                state_next = STATE_INIT;
            end
        end
        STATE_IDLE: begin
            // idle state
            write_addr_next = write_addr;
            write_data_padded_next = write_data_padded;
            write_delete_next = write_delete;

            if (write_enable) begin
                // wait for read from erase_ram
                state_next = STATE_DELETE_1;
            end else begin
                state_next = STATE_IDLE;
            end
        end
        STATE_DELETE_1: begin
            // wait for read
            state_next = STATE_DELETE_2;
        end
        STATE_DELETE_2: begin
            // clear bit and write back
            clear_bit = 1'b1 << write_addr;
            wr_en = 1'b1;
            if (write_delete_reg) begin
                state_next = STATE_IDLE;
            end else begin
                erase_ram_wr_en = 1'b1;
                state_next = STATE_WRITE_1;
            end
        end
        STATE_WRITE_1: begin
            // wait for read
            state_next = STATE_WRITE_2;
        end
        STATE_WRITE_2: begin
            // set bit and write back
            set_bit = 1'b1 << write_addr;
            wr_en = 1'b1;
            state_next = STATE_IDLE;
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
    write_delete_reg <= write_delete_next;
end

endmodule
