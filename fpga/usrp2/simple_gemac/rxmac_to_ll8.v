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


module rxmac_to_ll8
  (input clk, input reset, input clear,
   input [7:0] rx_data, input rx_valid, input rx_error, input rx_ack,
   output [7:0] ll_data, output ll_sof, output ll_eof, output ll_error, output ll_src_rdy, input ll_dst_rdy );

   reg [2:0] xfer_state;

   localparam XFER_IDLE     = 0;
   localparam XFER_ACTIVE   = 1;
   localparam XFER_ERROR    = 2;
   localparam XFER_ERROR2   = 3;
   localparam XFER_OVERRUN  = 4;
   localparam XFER_OVERRUN2 = 5;
      
   assign ll_data 	    = rx_data;
   assign ll_src_rdy 	    = ((rx_valid & (xfer_state != XFER_OVERRUN2) )
			       | (xfer_state == XFER_ERROR) 
			       | (xfer_state == XFER_OVERRUN));
   assign ll_sof 	    = ((xfer_state==XFER_IDLE)|(xfer_state==XFER_ERROR)|(xfer_state==XFER_OVERRUN));
   assign ll_eof 	    = (rx_ack | (xfer_state==XFER_ERROR) | (xfer_state==XFER_OVERRUN));
   assign ll_error 	    = (xfer_state == XFER_ERROR)|(xfer_state==XFER_OVERRUN);
   
   always @(posedge clk)
     if(reset | clear)
       xfer_state 	   <= XFER_IDLE;
     else
       case(xfer_state)
	 XFER_IDLE :
	   if(rx_valid)
	     xfer_state <= XFER_ACTIVE;
	 XFER_ACTIVE :
	   if(rx_error)
	     xfer_state <= XFER_ERROR;
	   else if(~rx_valid)
	     xfer_state <= XFER_IDLE;
	   else if(~ll_dst_rdy)
	     xfer_state <= XFER_OVERRUN;
	 XFER_ERROR :
	   if(ll_dst_rdy)
	     xfer_state <= XFER_ERROR2;
	 XFER_ERROR2 :
	   if(~rx_error)
	     xfer_state <= XFER_IDLE;
	 XFER_OVERRUN :
	   if(ll_dst_rdy)
	     xfer_state <= XFER_OVERRUN2;
	 XFER_OVERRUN2 :
	   if(~rx_valid)
	     xfer_state <= XFER_IDLE;
       endcase // case (xfer_state)

   
endmodule // rxmac_to_ll8
