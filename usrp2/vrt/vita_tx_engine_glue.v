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

//The following module is used to re-write transmit packets from the host.
//This module provides a packet-based ram interface for manipulating packets.
//By default, this module uses the built-in 8 to 16 bit converter engine.

module vita_tx_engine_glue
#(
    //the dsp unit number: 0, 1, 2...
    parameter DSPNO = 0,

    //buffer size for ram interface engine
    parameter BUF_SIZE = 10,

    //base address for built-in settings registers used in this module
    parameter MAIN_SETTINGS_BASE = 0,

    //the number of 32bit lines between start of buffer and vita header
    //the metadata before the header should be preserved by the engine
    parameter HEADER_OFFSET = 0
)
(
    //control signals
    input clock, input reset, input clear,

    //main settings bus for built-in modules
    input set_stb_main, input [7:0] set_addr_main, input [31:0] set_data_main,

    //user settings bus, controlled through user setting regs API
    input set_stb_user, input [7:0] set_addr_user, input [31:0] set_data_user,

    //ram interface for engine
    output access_we,
    output access_stb,
    input access_ok,
    output access_done,
    output access_skip_read,
    output [BUF_SIZE-1:0] access_adr,
    input [BUF_SIZE-1:0] access_len,
    output [35:0] access_dat_o,
    input [35:0] access_dat_i,

    //debug output (optional)
    output [31:0] debug
);

    generate
        if (DSPNO==0) begin
            `ifndef TX_ENG0_MODULE
            dspengine_8to16 #(.BASE(MAIN_SETTINGS_BASE), .BUF_SIZE(BUF_SIZE), .HEADER_OFFSET(HEADER_OFFSET)) dspengine_8to16
             (.clk(clock),.reset(reset),.clear(clear),
              .set_stb(set_stb_main), .set_addr(set_addr_main), .set_data(set_data_main),
              .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
              .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
              .access_dat_i(access_dat_i), .access_dat_o(access_dat_o));
            `else
            `TX_ENG0_MODULE #(.BUF_SIZE(BUF_SIZE), .HEADER_OFFSET(HEADER_OFFSET)) tx_eng0_custom
             (.clock(clock),.reset(reset),.clear(clear),
              .set_stb(set_stb_user), .set_addr(set_addr_user), .set_data(set_data_user),
              .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
              .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
              .access_dat_i(access_dat_i), .access_dat_o(access_dat_o));
            `endif
        end
        else begin
            `ifndef TX_ENG1_MODULE
            dspengine_8to16 #(.BASE(MAIN_SETTINGS_BASE), .BUF_SIZE(BUF_SIZE), .HEADER_OFFSET(HEADER_OFFSET)) dspengine_8to16
             (.clk(clock),.reset(reset),.clear(clear),
              .set_stb(set_stb_main), .set_addr(set_addr_main), .set_data(set_data_main),
              .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
              .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
              .access_dat_i(access_dat_i), .access_dat_o(access_dat_o));
            `else
            `TX_ENG1_MODULE #(.BUF_SIZE(BUF_SIZE), .HEADER_OFFSET(HEADER_OFFSET)) tx_eng1_custom
             (.clock(clock),.reset(reset),.clear(clear),
              .set_stb(set_stb_user), .set_addr(set_addr_user), .set_data(set_data_user),
              .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
              .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
              .access_dat_i(access_dat_i), .access_dat_o(access_dat_o));
            `endif
        end
    endgenerate

endmodule //vita_tx_engine_glue
