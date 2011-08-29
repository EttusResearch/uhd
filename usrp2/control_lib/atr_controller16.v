//
// Copyright 2011 Ettus Research LLC
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


// Automatic transmit/receive switching of control pins to daughterboards
// Store everything in registers for now, but could use a RAM for more
// complex state machines in the future

module atr_controller16
  (input clk_i, input rst_i,
   input [5:0] adr_i, input [1:0] sel_i, input [15:0] dat_i, output [15:0] dat_o,
   input we_i, input stb_i, input cyc_i, output reg ack_o,
   input run_rx, input run_tx,
   output [31:0] ctrl_lines);
   
   reg [3:0] state;
   reg [31:0] atr_ram [0:15];  // DP distributed RAM

   wire [3:0] sel_int = { (sel_i[1] & adr_i[1]), (sel_i[0] & adr_i[1]),
			  (sel_i[1] & ~adr_i[1]), (sel_i[0] & ~adr_i[1]) };
   
   // WB Interface
   always @(posedge clk_i)
     if(we_i & stb_i & cyc_i)
       begin
	  if(sel_int[3])
	    atr_ram[adr_i[5:2]][31:24] <= dat_i[15:8];
	  if(sel_int[2])
	    atr_ram[adr_i[5:2]][23:16] <= dat_i[7:0];
	  if(sel_int[1])
	    atr_ram[adr_i[5:2]][15:8] <= dat_i[15:8];
	  if(sel_int[0])
	    atr_ram[adr_i[5:2]][7:0] <= dat_i[7:0];
       end // if (we_i & stb_i & cyc_i)

   // Removing readback allows ram to be synthesized as LUTs instead of regs
   //always @(posedge clk_i)
   //  dat_o <= adr_i[1] ? atr_ram[adr_i[5:2]][31:16] : atr_ram[adr_i[5:2]][15:0];
   assign dat_o = 16'd0;
   
   always @(posedge clk_i)
     ack_o <= stb_i & cyc_i & ~ack_o;

   // Control side of DP RAM
   assign     ctrl_lines = atr_ram[state];

   // Put a more complex state machine with time delays and multiple states here
   //  if daughterboard requires more complex sequencing
   localparam ATR_IDLE = 4'd0;
   localparam ATR_TX = 4'd1;
   localparam ATR_RX = 4'd2;
   localparam ATR_FULL_DUPLEX = 4'd3;
   
   always @(posedge clk_i)
     if(rst_i)
       state <= ATR_IDLE;
     else
       case ({run_rx,run_tx})
	 2'b00 : state <= ATR_IDLE;
	 2'b01 : state <= ATR_TX;
	 2'b10 : state <= ATR_RX;
	 2'b11 : state <= ATR_FULL_DUPLEX;
       endcase // case({run_rx,run_tx})
   
endmodule // atr_controller16
