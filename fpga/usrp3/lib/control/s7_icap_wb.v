//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
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

//
// Refer to SelectMAP and ICAP docs in UG470
//

module s7_icap_wb
  (
   input clk, 
   input reset,
   input cyc_i, 
   input stb_i, 
   input we_i, 
   output ack_o,
   input [31:0] dat_i, 
   output [31:0] dat_o
   );

   reg 		 rdwrb, csib;
   

   reg [2:0]    icap_state;
   localparam ICAP_IDLE  = 0;
   localparam ICAP_WR0   = 1;
   localparam ICAP_WR1   = 2;
   localparam ICAP_RD0   = 3;
   localparam ICAP_RD1   = 4;

   localparam IDLE = 1'b1;
   localparam ACTIVE = 1'b0;
   localparam READ = 1'b1;
   localparam WRITE = 1'b0;
   

   always @(posedge clk)
     if(reset) begin
	rdwrb <= READ;
	csib <= IDLE;
       icap_state <= ICAP_IDLE;
     end
     else
       case(icap_state)
	 //
	 // In IDLE state waiting for a READ or WRITE to be signalled from the WB bus.
	 // (In this state rdwrb can flip state without effect because ICAP is not selected)
	 //
         ICAP_IDLE :
           begin
              if(stb_i & cyc_i) begin
                 if(we_i) begin
		    // Start WRITE, assert RDWR_B LOW whilst CSI_B remains HIGH.
		    rdwrb <= WRITE;
		    csib <= IDLE;
		    icap_state <= ICAP_WR0;
		 end else begin
		    // Start READ
		    rdwrb <= READ;
		    csib <= IDLE;
		    icap_state <= ICAP_RD0;
		 end
	      end else begin
		 // Stay IDLE
		 rdwrb <= READ;
		 csib <= IDLE;
		 icap_state <= ICAP_IDLE;
	      end
	   end // case: ICAP_IDLE
	 //
	 // First cycle of WRITE.  
	 // Next cycle assert RDWR_B LOW and assert CSI_B LOW. 
	 //
         ICAP_WR0 : begin
	    rdwrb <= WRITE;
	    csib <= ACTIVE;
            icap_state <= ICAP_WR1;
	 end
	 //
	 // Second cycle of WRITE.
	 // Next cycle assert RDWR_B LOW and assert CSI_B HIGH whilst transitioning to IDLE state
	 //
         ICAP_WR1 : begin
	    rdwrb <= WRITE;
	    csib <= IDLE;
            icap_state <= ICAP_IDLE;
	 end
	 //
	 // First cycle of READ.
	 // Next cycle assert RDWR_B HIGH and assert CSI_B LOW. 
	 //
         ICAP_RD0 : begin
            rdwrb <= READ;
	    csib <= ACTIVE;
            icap_state <= ICAP_WR1;
	 end
	 //
	 // Second cycle of READ.
	 // Next cycle assert RDWR_B HIGH and assert CSI_B HIGH whilst transitioning to IDLE state
	 //
         ICAP_RD1 : begin
	    rdwrb <= READ;
	    csib <= IDLE;
            icap_state <= ICAP_IDLE;
	 end
        
       endcase // case (icap_state)

   assign ack_o = (icap_state == ICAP_WR1) | (icap_state == ICAP_RD1);
   //assign debug_out = {17'd0, BUSY, dat_i[7:0], ~CE, ICAPCLK, ~WRITE, icap_state};
 

   ICAPE2  #(
	     .DEVICE_ID(32'h03651093),
	     .ICAP_WIDTH("X32"),
	     .SIM_CFG_FILE_NAME("NONE")
	     )
     ICAPE2_inst  (
		   .O(/*dat_o[31:0]*/),
		   .CLK(clk),     // Rising edge referenced for both reads and writes.
		   .CSIB(csib),        // CSIB = 0 to select ICAP
		   .I(dat_i[31:0]),   // Bitswaped as per SELECTMAP (See UG470 page 40)
		   .RDWRB(rdwrb)     // RDWB = 0 for WRITE, = 1 for READ
		   );
   
   assign dat_0 = 32'h0;
   
endmodule // s3a_icap_wb
