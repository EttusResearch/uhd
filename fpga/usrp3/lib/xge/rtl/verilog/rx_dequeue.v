//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "rx_dequeue.v"                                    ////
////                                                              ////
////  This file is part of the "10GE MAC" project                 ////
////  http://www.opencores.org/cores/xge_mac/                     ////
////                                                              ////
////  Author(s):                                                  ////
////      - A. Tanguay (antanguay@opencores.org)                  ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
////                                                              ////
//// Copyright (C) 2008 AUTHORS. All rights reserved.             ////
////                                                              ////
//// This source file may be used and distributed without         ////
//// restriction provided that this copyright statement is not    ////
//// removed from the file and that any derivative work contains  ////
//// the original copyright notice and the associated disclaimer. ////
////                                                              ////
//// This source file is free software; you can redistribute it   ////
//// and/or modify it under the terms of the GNU Lesser General   ////
//// Public License as published by the Free Software Foundation; ////
//// either version 2.1 of the License, or (at your option) any   ////
//// later version.                                               ////
////                                                              ////
//// This source is distributed in the hope that it will be       ////
//// useful, but WITHOUT ANY WARRANTY; without even the implied   ////
//// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      ////
//// PURPOSE.  See the GNU Lesser General Public License for more ////
//// details.                                                     ////
////                                                              ////
//// You should have received a copy of the GNU Lesser General    ////
//// Public License along with this source; if not, download it   ////
//// from http://www.opencores.org/lgpl.shtml                     ////
////                                                              ////
//////////////////////////////////////////////////////////////////////
 
 
`include "defines.v"
 
module rx_dequeue(/*AUTOARG*/
  // Outputs
  rxdfifo_ren, pkt_rx_data, pkt_rx_val, pkt_rx_sop, pkt_rx_eop,
  pkt_rx_err, pkt_rx_mod, pkt_rx_avail, status_rxdfifo_udflow_tog,
  // Inputs
  clk_156m25, reset_156m25_n, rxdfifo_rdata, rxdfifo_rstatus,
  rxdfifo_rempty, rxdfifo_ralmost_empty, pkt_rx_ren
  );
 
input         clk_156m25;
input         reset_156m25_n;
 
input [63:0]  rxdfifo_rdata;
input [7:0]   rxdfifo_rstatus;
input         rxdfifo_rempty;
input         rxdfifo_ralmost_empty;
 
input         pkt_rx_ren;
 
output        rxdfifo_ren;
 
output [63:0] pkt_rx_data;
output        pkt_rx_val;
output        pkt_rx_sop;
output        pkt_rx_eop;
output        pkt_rx_err;
output [2:0]  pkt_rx_mod;
output        pkt_rx_avail;
 
output        status_rxdfifo_udflow_tog;
 
/*AUTOREG*/
// Beginning of automatic regs (for this module's undeclared outputs)
reg                     pkt_rx_avail;
reg [63:0]              pkt_rx_data;
reg                     pkt_rx_eop;
reg                     pkt_rx_err;
reg [2:0]               pkt_rx_mod;
reg                     pkt_rx_sop;
reg                     pkt_rx_val;
reg                     status_rxdfifo_udflow_tog;
// End of automatics
 
reg           end_eop;
 
/*AUTOWIRE*/
 
 
// End eop to force one cycle between packets
 
assign rxdfifo_ren = !rxdfifo_rempty && pkt_rx_ren && !end_eop;
 
 
 
always @(posedge clk_156m25 or negedge reset_156m25_n) begin
 
    if (reset_156m25_n == 1'b0) begin
 
        pkt_rx_avail <= 1'b0;
 
        pkt_rx_data <= 64'b0;
        pkt_rx_sop <= 1'b0;
        pkt_rx_eop <= 1'b0;
        pkt_rx_err <= 1'b0;
        pkt_rx_mod <= 3'b0;
 
        pkt_rx_val <= 1'b0;
 
        end_eop <= 1'b0;
 
        status_rxdfifo_udflow_tog <= 1'b0;
 
    end
    else begin
 
        pkt_rx_avail <= !rxdfifo_ralmost_empty;
 
 
 
        // If eop shows up at the output of the fifo, we drive eop on
        // the bus on the next read. This will be the last read for this
        // packet. The fifo is designed to output data early. On last read,
        // data from next packet will appear at the output of fifo. Modulus
        // of packet length is in lower bits.
 
        pkt_rx_eop <= rxdfifo_ren && rxdfifo_rstatus[`RXSTATUS_EOP];
        pkt_rx_mod <= {3{rxdfifo_ren & rxdfifo_rstatus[`RXSTATUS_EOP]}} & rxdfifo_rstatus[2:0];
 
 
        pkt_rx_val <= rxdfifo_ren;
 
        if (rxdfifo_ren) begin
 
            `ifdef BIGENDIAN
	    pkt_rx_data <= {rxdfifo_rdata[7:0],
                            rxdfifo_rdata[15:8],
                            rxdfifo_rdata[23:16],
                            rxdfifo_rdata[31:24],
                            rxdfifo_rdata[39:32],
                            rxdfifo_rdata[47:40],
                            rxdfifo_rdata[55:48],
                            rxdfifo_rdata[63:56]};
            `else
	    pkt_rx_data <= rxdfifo_rdata;
            `endif
 
        end
 
 
        if (rxdfifo_ren && rxdfifo_rstatus[`RXSTATUS_SOP]) begin
 
            // SOP indication on first word
 
            pkt_rx_sop <= 1'b1;
            pkt_rx_err <= 1'b0;
 
        end
        else begin
 
            pkt_rx_sop <= 1'b0;
 
 
            // Give an error if FIFO is to underflow
 
            if (rxdfifo_rempty && pkt_rx_ren && !end_eop) begin
                pkt_rx_val <= 1'b1;
                pkt_rx_eop <= 1'b1;
                pkt_rx_err <= 1'b1;
            end
 
        end
 
 
        if (rxdfifo_ren && |(rxdfifo_rstatus[`RXSTATUS_ERR])) begin
 
            // Status stored in FIFO is propagated to error signal.
 
            pkt_rx_err <= 1'b1;
 
        end
 
 
        //---
        // EOP indication at the end of the frame. Cleared otherwise.
 
        if (rxdfifo_ren && rxdfifo_rstatus[`RXSTATUS_EOP]) begin
            end_eop <= 1'b1;
        end
        else if (pkt_rx_ren) begin
            end_eop <= 1'b0;
        end
 
 
 
        //---
        // FIFO errors, used to generate interrupts
 
        if (rxdfifo_rempty && pkt_rx_ren && !end_eop) begin
            status_rxdfifo_udflow_tog <= ~status_rxdfifo_udflow_tog;
        end
 
    end
end
 
endmodule
