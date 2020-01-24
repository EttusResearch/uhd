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
 * Content Addressable Memory
 */
module cam #(
    // search data bus width
    parameter DATA_WIDTH = 64,
    // memory size in log2(words)
    parameter ADDR_WIDTH = 5,
    // CAM style (SRL, BRAM)
    parameter CAM_STYLE = "SRL",
    // width of data bus slices
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

generate
    if (CAM_STYLE == "SRL") begin
        cam_srl #(
            .DATA_WIDTH(DATA_WIDTH),
            .ADDR_WIDTH(ADDR_WIDTH),
            .SLICE_WIDTH(SLICE_WIDTH)
        )
        cam_inst (
            .clk(clk),
            .rst(rst),
            .write_addr(write_addr),
            .write_data(write_data),
            .write_delete(write_delete),
            .write_enable(write_enable),
            .write_busy(write_busy),
            .compare_data(compare_data),
            .match_many(match_many),
            .match_single(match_single),
            .match_addr(match_addr),
            .match(match)
        );
    end else if (CAM_STYLE == "BRAM") begin
        cam_bram #(
            .DATA_WIDTH(DATA_WIDTH),
            .ADDR_WIDTH(ADDR_WIDTH),
            .SLICE_WIDTH(SLICE_WIDTH)
        )
        cam_inst (
            .clk(clk),
            .rst(rst),
            .write_addr(write_addr),
            .write_data(write_data),
            .write_delete(write_delete),
            .write_enable(write_enable),
            .write_busy(write_busy),
            .compare_data(compare_data),
            .match_many(match_many),
            .match_single(match_single),
            .match_addr(match_addr),
            .match(match)
        );
    end
endgenerate

endmodule
