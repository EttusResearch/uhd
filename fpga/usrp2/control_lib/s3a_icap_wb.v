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



module s3a_icap_wb
  (input clk, input reset,
   input cyc_i, input stb_i, input we_i, output ack_o,
   input [31:0] dat_i, output [31:0] dat_o);//, output [31:0] debug_out);

   assign dat_o[31:8] = 24'd0;
   
   wire 	BUSY, CE, WRITE, ICAPCLK;
   
   //changed this to gray-ish code to prevent glitching
   reg [2:0] 	icap_state;
   localparam ICAP_IDLE  = 0;
   localparam ICAP_WR0 	 = 1;
   localparam ICAP_WR1 	 = 5;
   localparam ICAP_RD0 	 = 2;
   localparam ICAP_RD1 	 = 3;

   always @(posedge clk)
     if(reset)
       icap_state 	<= ICAP_IDLE;
     else
       case(icap_state)
	 ICAP_IDLE :
	   begin
	      if(stb_i & cyc_i)
		if(we_i)
		  icap_state <= ICAP_WR0;
		else
		  icap_state <= ICAP_RD0;
	   end
	 ICAP_WR0 :
	   icap_state <= ICAP_WR1;
	 ICAP_WR1 :
	   icap_state <= ICAP_IDLE;
	 ICAP_RD0 :
	   icap_state <= ICAP_RD1;
	 ICAP_RD1 :
	   icap_state <= ICAP_IDLE;
       endcase // case (icap_state)

   assign WRITE 	 = (icap_state == ICAP_WR0) | (icap_state == ICAP_WR1);
   assign CE 		 = (icap_state == ICAP_WR0) | (icap_state == ICAP_RD0);
   assign ICAPCLK        = CE & (~clk);

   assign ack_o = (icap_state == ICAP_WR1) | (icap_state == ICAP_RD1);
   //assign debug_out = {17'd0, BUSY, dat_i[7:0], ~CE, ICAPCLK, ~WRITE, icap_state};
   
   ICAP_SPARTAN3A ICAP_SPARTAN3A_inst 
     (.BUSY(BUSY),          // Busy output
      .O(dat_o[7:0]),            // 32-bit data output
      .CE(~CE),              // Clock enable input
      .CLK(ICAPCLK),            // Clock input
      .I(dat_i[7:0]),            // 32-bit data input
      .WRITE(~WRITE)         // Write input
      );

endmodule // s3a_icap_wb
