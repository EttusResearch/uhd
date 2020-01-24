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


module ll8_to_txmac
  (input clk, input reset, input clear,
   input [7:0] ll_data, input ll_sof, input ll_eof, input ll_src_rdy, output ll_dst_rdy,
   output [7:0] tx_data, output tx_valid, output tx_error, input tx_ack );

   reg [2:0] xfer_state;

   localparam XFER_IDLE      = 0;
   localparam XFER_ACTIVE    = 1;
   localparam XFER_WAIT1     = 2;
   localparam XFER_UNDERRUN  = 3;
   localparam XFER_DROP      = 4;
   
   always @(posedge clk)
     if(reset | clear)
       xfer_state 	    <= XFER_IDLE;
     else
       case(xfer_state)
	 XFER_IDLE :
	   if(tx_ack)
	     xfer_state <= XFER_ACTIVE;
	 XFER_ACTIVE :
	   if(~ll_src_rdy)
	     xfer_state <= XFER_UNDERRUN;
	   else if(ll_eof)
	     xfer_state <= XFER_WAIT1;
	 XFER_WAIT1 :
	   xfer_state <= XFER_IDLE;
	 XFER_UNDERRUN :
	   xfer_state <= XFER_DROP;
	 XFER_DROP :
	   if(ll_eof)
	     xfer_state <= XFER_IDLE;
       endcase // case (xfer_state)

   assign ll_dst_rdy 	 = (xfer_state == XFER_ACTIVE) | tx_ack | (xfer_state == XFER_DROP);
   assign tx_valid 	 = (ll_src_rdy & (xfer_state == XFER_IDLE))|(xfer_state == XFER_ACTIVE);
   assign tx_data 	 = ll_data;
   assign tx_error 	 = (xfer_state == XFER_UNDERRUN);
   
endmodule // ll8_to_txmac

