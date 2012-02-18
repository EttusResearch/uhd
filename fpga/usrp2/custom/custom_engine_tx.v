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

//COPY ME, CUSTOMIZE ME...

//The following module is used to re-write transmit packets from the host.
//This module provides a packet-based ram interface for manipulating packets.
//The user writes a custom engine (state machine) to read the input packet,
//and to produce a new output packet.

//By default, this entire module is a simple pass-through.

module custom_engine_tx
#(
    //buffer size for ram interface engine
    parameter BUF_SIZE = 10,

    //the number of 32bit lines between start of buffer and vita header
    //the metadata before the header should be preserved by the engine
    parameter HEADER_OFFSET = 0
)
(
    //control signals
    input clock, input reset, input clear,

    //user settings bus, controlled through user setting regs API
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    //ram interface for engine
    output access_we,
    output access_stb,
    input access_ok,
    output access_done,
    output access_skip_read,
    output [BUF_SIZE-1:0] access_adr,
    input [BUF_SIZE-1:0] access_len,
    output [35:0] access_dat_o,
    input [35:0] access_dat_i
);

    assign access_done = access_ok;

endmodule //custom_engine_tx
