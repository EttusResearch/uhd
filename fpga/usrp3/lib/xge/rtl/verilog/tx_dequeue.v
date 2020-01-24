//////////////////////////////////////////////////////////////////////
////                                                              ////
////  File name "tx_dequeue.v"                                    ////
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
 
module tx_dequeue(/*AUTOARG*/
  // Outputs
  txdfifo_ren, txhfifo_ren, txhfifo_wdata, txhfifo_wstatus,
  txhfifo_wen, xgmii_txd, xgmii_txc, status_txdfifo_udflow_tog,
  // Inputs
  clk_xgmii_tx, reset_xgmii_tx_n, ctrl_tx_enable_ctx,
  status_local_fault_ctx, status_remote_fault_ctx, txdfifo_rdata,
  txdfifo_rstatus, txdfifo_rempty, txdfifo_ralmost_empty,
  txhfifo_rdata, txhfifo_rstatus, txhfifo_rempty,
  txhfifo_ralmost_empty, txhfifo_wfull, txhfifo_walmost_full
  );
`include "CRC32_D64.v"
`include "CRC32_D8.v"
`include "utils.v"
 
input         clk_xgmii_tx;
input         reset_xgmii_tx_n;
 
input         ctrl_tx_enable_ctx;
 
input         status_local_fault_ctx;
input         status_remote_fault_ctx;
 
input  [63:0] txdfifo_rdata;
input  [7:0]  txdfifo_rstatus;
input         txdfifo_rempty;
input         txdfifo_ralmost_empty;
 
input  [63:0] txhfifo_rdata;
input  [7:0]  txhfifo_rstatus;
input         txhfifo_rempty;
input         txhfifo_ralmost_empty;
 
input         txhfifo_wfull;
input         txhfifo_walmost_full;
 
output        txdfifo_ren;
 
output        txhfifo_ren;
 
output [63:0] txhfifo_wdata;
output [7:0]  txhfifo_wstatus;
output        txhfifo_wen;   
 
output [63:0] xgmii_txd;
output [7:0]  xgmii_txc;
 
output        status_txdfifo_udflow_tog;
 
 
 
 
/*AUTOREG*/
// Beginning of automatic regs (for this module's undeclared outputs)
reg                     status_txdfifo_udflow_tog;
reg                     txdfifo_ren;
reg                     txhfifo_ren;
reg [63:0]              txhfifo_wdata;
reg                     txhfifo_wen;
reg [7:0]               txhfifo_wstatus;
reg [7:0]               xgmii_txc;
reg [63:0]              xgmii_txd;
// End of automatics
 
/*AUTOWIRE*/
 
 
reg   [63:0]    xgxs_txd;
reg   [7:0]     xgxs_txc;
 
reg   [63:0]    next_xgxs_txd;
reg   [7:0]     next_xgxs_txc;
 
reg   [2:0]     curr_state_enc;
reg   [2:0]     next_state_enc;
 
reg   [0:0]     curr_state_pad;
reg   [0:0]     next_state_pad;
 
reg             start_on_lane0;
reg             next_start_on_lane0;
 
reg   [2:0]     ifg_deficit;
reg   [2:0]     next_ifg_deficit;
 
reg             ifg_4b_add;
reg             next_ifg_4b_add;
 
reg             ifg_8b_add;
reg             next_ifg_8b_add;
 
reg             ifg_8b2_add;
reg             next_ifg_8b2_add;
 
reg   [7:0]     eop;
reg   [7:0]     next_eop;
 
reg   [63:32]   xgxs_txd_barrel;
reg   [7:4]     xgxs_txc_barrel;
 
reg   [63:0]    txhfifo_rdata_d1;
 
reg   [13:0]    byte_cnt;
 
reg   [31:0]    crc32_d64;
reg   [31:0]    crc32_d8;
reg   [31:0]    crc32_tx;
 
reg   [63:0]    shift_crc_data;
reg   [3:0]     shift_crc_eop;
reg   [3:0]     shift_crc_cnt;
 
reg   [31:0]    crc_data;
 
reg             frame_available;
reg             next_frame_available;
 
reg   [63:0]    next_txhfifo_wdata;
reg   [7:0]     next_txhfifo_wstatus;
reg             next_txhfifo_wen;   
 
reg             txdfifo_ren_d1;
 
parameter [2:0]
             SM_IDLE      = 3'd0,
             SM_PREAMBLE  = 3'd1,
             SM_TX        = 3'd2,
             SM_EOP       = 3'd3,
             SM_TERM      = 3'd4,
             SM_TERM_FAIL = 3'd5,
             SM_IFG       = 3'd6;
 
parameter [0:0]
             SM_PAD_EQ    = 1'd0,
             SM_PAD_PAD   = 1'd1;
 
 
//---
// RC layer
 
always @(posedge clk_xgmii_tx or negedge reset_xgmii_tx_n) begin
 
    if (reset_xgmii_tx_n == 1'b0) begin
 
        xgmii_txd <= {8{`IDLE}};
        xgmii_txc <= 8'hff;
 
    end
    else begin
 
 
        //---
        // RC Layer, insert local or remote fault messages based on status
        // of fault state-machine
 
        if (status_local_fault_ctx) begin
 
            // If local fault detected, send remote fault message to
            // link partner
 
            xgmii_txd <= {`REMOTE_FAULT, 8'h0, 8'h0, `SEQUENCE,
                          `REMOTE_FAULT, 8'h0, 8'h0, `SEQUENCE};
            xgmii_txc <= {4'b0001, 4'b0001};
        end
        else if (status_remote_fault_ctx) begin
 
            // If remote fault detected, inhibit transmission and send
            // idle codes
 
            xgmii_txd <= {8{`IDLE}};
            xgmii_txc <= 8'hff;
        end
        else begin
            xgmii_txd <= xgxs_txd;
            xgmii_txc <= xgxs_txc;
        end
    end
 
end
 
 
always @(posedge clk_xgmii_tx or negedge reset_xgmii_tx_n) begin
 
    if (reset_xgmii_tx_n == 1'b0) begin
 
        curr_state_enc <= SM_IDLE;
 
        start_on_lane0 <= 1'b1;
        ifg_deficit <= 3'b0;
        ifg_4b_add <= 1'b0;
        ifg_8b_add <= 1'b0;
        ifg_8b2_add <= 1'b0;
 
        eop <= 8'b0;
 
        txhfifo_rdata_d1 <= 64'b0;
 
        xgxs_txd_barrel <= {4{`IDLE}};
        xgxs_txc_barrel <= 4'hf;
 
        frame_available <= 1'b0;
 
        xgxs_txd <= {8{`IDLE}};
        xgxs_txc <= 8'hff;
 
        status_txdfifo_udflow_tog <= 1'b0;
 
    end
    else begin
 
        curr_state_enc <= next_state_enc;
 
        start_on_lane0 <= next_start_on_lane0;
        ifg_deficit <= next_ifg_deficit;
        ifg_4b_add <= next_ifg_4b_add;
        ifg_8b_add <= next_ifg_8b_add;
        ifg_8b2_add <= next_ifg_8b2_add;
 
        eop <= next_eop;
 
        txhfifo_rdata_d1 <= txhfifo_rdata;
 
        xgxs_txd_barrel <= next_xgxs_txd[63:32];
        xgxs_txc_barrel <= next_xgxs_txc[7:4];
 
        frame_available <= next_frame_available;
 
        //---
        // Barrel shifter. Previous stage always align packet with LANE0.
        // This stage allow us to shift packet to align with LANE4 if needed
        // for correct inter frame gap (IFG).
 
        if (next_start_on_lane0) begin
 
            xgxs_txd <= next_xgxs_txd;
            xgxs_txc <= next_xgxs_txc;
 
        end
        else begin
 
            xgxs_txd <= {next_xgxs_txd[31:0], xgxs_txd_barrel};
            xgxs_txc <= {next_xgxs_txc[3:0], xgxs_txc_barrel};
 
        end
 
        //---
        // FIFO errors, used to generate interrupts.
 
        if (txdfifo_ren && txdfifo_rempty) begin
            status_txdfifo_udflow_tog <= ~status_txdfifo_udflow_tog;
        end
 
    end
 
end
 
always @(/*AS*/crc32_tx or ctrl_tx_enable_ctx or curr_state_enc or eop
         or frame_available or ifg_4b_add or ifg_8b2_add or ifg_8b_add
         or ifg_deficit or start_on_lane0 or status_local_fault_ctx
         or txhfifo_ralmost_empty or txhfifo_rdata_d1
         or txhfifo_rempty or txhfifo_rstatus) begin
 
    next_state_enc = curr_state_enc;
 
    next_start_on_lane0 = start_on_lane0;
    next_ifg_deficit = ifg_deficit;
    next_ifg_4b_add = ifg_4b_add;
    next_ifg_8b_add = ifg_8b_add;
    next_ifg_8b2_add = ifg_8b2_add;
 
    next_eop = eop;
 
    next_xgxs_txd = {8{`IDLE}};
    next_xgxs_txc = 8'hff;
 
    txhfifo_ren = 1'b0;
 
    next_frame_available = frame_available;
 
    case (curr_state_enc)
 
        SM_IDLE:
          begin
 
              // Wait for frame to be available. There should be a least N bytes in the
              // data fifo or a crc in the control fifo. The N bytes in the data fifo
              // give time to the enqueue engine to calculate crc and write it to the
              // control fifo. If crc is already in control fifo we can start transmitting
              // with no concern. Transmission is inhibited if local or remote faults
              // are detected.
 
              if (ctrl_tx_enable_ctx && frame_available &&
                  !status_local_fault_ctx && !status_local_fault_ctx) begin
 
                  txhfifo_ren = 1'b1;
                  next_state_enc = SM_PREAMBLE;
 
              end
              else begin
 
                  next_frame_available = !txhfifo_ralmost_empty;
                  next_ifg_4b_add = 1'b0;
 
              end
 
          end
 
        SM_PREAMBLE:
         begin
 
             // On reading SOP from fifo, send SFD and preamble characters
 
             if (txhfifo_rstatus[`TXSTATUS_SOP]) begin
 
                 next_xgxs_txd = {`SFD, {6{`PREAMBLE}}, `START};
                 next_xgxs_txc = 8'h01;
 
                 txhfifo_ren = 1'b1;
 
                 next_state_enc = SM_TX;
 
             end
             else begin
 
                 next_frame_available = 1'b0;
                 next_state_enc = SM_IDLE;
 
             end
 
 
             // Depending on deficit idle count calculations, add 4 bytes
             // or IFG or not. This will determine on which lane start the
             // next frame.
 
             if (ifg_4b_add) begin
                 next_start_on_lane0 = 1'b0;
             end
             else begin
                 next_start_on_lane0 = 1'b1;
             end
 
          end
 
        SM_TX:
          begin
 
              next_xgxs_txd = txhfifo_rdata_d1;
              next_xgxs_txc = 8'h00;
 
              txhfifo_ren = 1'b1;
 
 
              // Wait for EOP indication to be read from the fifo, then
              // transition to next state.
 
              if (txhfifo_rstatus[`TXSTATUS_EOP]) begin
 
                  txhfifo_ren = 1'b0;
                  next_frame_available = !txhfifo_ralmost_empty;
                  next_state_enc = SM_EOP;
 
              end
              else if (txhfifo_rempty || txhfifo_rstatus[`TXSTATUS_SOP]) begin
 
                  // Failure condition, we did not see EOP and there
                  // is no more data in fifo or SOP, force end of packet transmit.
 
                  next_state_enc = SM_TERM_FAIL;
 
              end
 
              next_eop[0] = txhfifo_rstatus[2:0] == 3'd1;
              next_eop[1] = txhfifo_rstatus[2:0] == 3'd2;
              next_eop[2] = txhfifo_rstatus[2:0] == 3'd3;
              next_eop[3] = txhfifo_rstatus[2:0] == 3'd4;
              next_eop[4] = txhfifo_rstatus[2:0] == 3'd5;
              next_eop[5] = txhfifo_rstatus[2:0] == 3'd6;
              next_eop[6] = txhfifo_rstatus[2:0] == 3'd7;
              next_eop[7] = txhfifo_rstatus[2:0] == 3'd0;
 
          end
 
        SM_EOP:
          begin
 
              // Insert TERMINATE character in correct lane depending on position
              // of EOP read from fifo. Also insert CRC read from control fifo.
 
              if (eop[0]) begin
                  next_xgxs_txd = {{2{`IDLE}}, `TERMINATE, 
                                   crc32_tx[31:0], txhfifo_rdata_d1[7:0]};
                  next_xgxs_txc = 8'b11100000;
              end
 
              if (eop[1]) begin
                  next_xgxs_txd = {`IDLE, `TERMINATE,
                                   crc32_tx[31:0], txhfifo_rdata_d1[15:0]};
                  next_xgxs_txc = 8'b11000000;
              end
 
              if (eop[2]) begin
                  next_xgxs_txd = {`TERMINATE, crc32_tx[31:0], txhfifo_rdata_d1[23:0]};
                  next_xgxs_txc = 8'b10000000;
              end
 
              if (eop[3]) begin
                  next_xgxs_txd = {crc32_tx[31:0], txhfifo_rdata_d1[31:0]};
                  next_xgxs_txc = 8'b00000000;
              end
 
              if (eop[4]) begin
                  next_xgxs_txd = {crc32_tx[23:0], txhfifo_rdata_d1[39:0]};
                  next_xgxs_txc = 8'b00000000;
              end
 
              if (eop[5]) begin
                  next_xgxs_txd = {crc32_tx[15:0], txhfifo_rdata_d1[47:0]};
                  next_xgxs_txc = 8'b00000000;
              end
 
              if (eop[6]) begin
                  next_xgxs_txd = {crc32_tx[7:0], txhfifo_rdata_d1[55:0]};
                  next_xgxs_txc = 8'b00000000;
              end
 
              if (eop[7]) begin
                  next_xgxs_txd = {txhfifo_rdata_d1[63:0]};
                  next_xgxs_txc = 8'b00000000;
              end
 
              if (!frame_available) begin
 
                  // If there is not another frame ready to be transmitted, interface
                  // will go idle and idle deficit idle count calculation is irrelevant.
                  // Set deficit to 0.
 
                  next_ifg_deficit = 3'b0;
 
              end
              else begin
 
                  // Idle deficit count calculated based on number of "wasted" bytes
                  // between TERMINATE and alignment of next frame in LANE0.
 
                  next_ifg_deficit = ifg_deficit +
                                     {2'b0, eop[0] | eop[4]} +
                                     {1'b0, eop[1] | eop[5], 1'b0} +
                                     {1'b0, eop[2] | eop[6],
                                      eop[2] | eop[6]};
              end
 
              // IFG corrections based on deficit count and previous starting lane
              // Calculated based on following table:
              //
              //                 DIC=0          DIC=1          DIC=2          DIC=3
              //              -------------  -------------  -------------  -------------
              // PktLen       IFG      Next  IFG      Next  IFG      Next  IFG      Next
              // Modulus      Length   DIC   Length   DIC   Length   DIC   Length   DIC
              // -----------------------------------------------------------------------
              //    0           12      0      12      1      12      2      12      3
              //    1           11      1      11      2      11      3      15      0
              //    2           10      2      10      3      14      0      14      1
              //    3            9      3      13      0      13      1      13      2
              //
              //
              // In logic it translates into adding 4, 8, or 12 bytes of IFG relative
              // to LANE0.
              //   IFG and Add columns assume no deficit applied
              //   IFG+DIC and Add+DIC assume deficit must be applied
              //
              //                        Start lane 0       Start lane 4	
              // EOP Pads IFG  IFG+DIC	Add   Add+DIC	   Add    Add IFG
              // 0   3    11   15        8     12           12     16
              // 1   2    10   14        8     12           12     16
              // 2   1    9    13        8     12           12     16
              // 3   8    12   12        4     4            8      8
              // 4   7    11   15        4     8            8      12
              // 5   6    10   14        4     8            8      12
              // 6   5    9    13        4     8            8      12
              // 7   4    12   12        8     8            12     12
 
              if (!frame_available) begin
 
                  // If there is not another frame ready to be transmitted, interface
                  // will go idle and idle deficit idle count calculation is irrelevant.
 
                  next_ifg_4b_add = 1'b0;
                  next_ifg_8b_add = 1'b0;
                  next_ifg_8b2_add = 1'b0;
 
              end
              else if (next_ifg_deficit[2] == ifg_deficit[2]) begin
 
                  // Add 4 bytes IFG
 
                  next_ifg_4b_add = (eop[0] & !start_on_lane0) |
                                    (eop[1] & !start_on_lane0) |
                                    (eop[2] & !start_on_lane0) |
                                    (eop[3] & start_on_lane0) |
                                    (eop[4] & start_on_lane0) |
                                    (eop[5] & start_on_lane0) |
                                    (eop[6] & start_on_lane0) |
                                    (eop[7] & !start_on_lane0);
 
                  // Add 8 bytes IFG
 
                  next_ifg_8b_add = (eop[0]) |
                                    (eop[1]) |
                                    (eop[2]) |
                                    (eop[3] & !start_on_lane0) |
                                    (eop[4] & !start_on_lane0) |
                                    (eop[5] & !start_on_lane0) |
                                    (eop[6] & !start_on_lane0) |
                                    (eop[7]);
 
                  // Add another 8 bytes IFG
 
                  next_ifg_8b2_add = 1'b0;
 
              end
              else begin
 
                  // Add 4 bytes IFG
 
                  next_ifg_4b_add = (eop[0] & start_on_lane0) |
                                    (eop[1] & start_on_lane0) |
                                    (eop[2] & start_on_lane0) |
                                    (eop[3] &  start_on_lane0) |
                                    (eop[4] & !start_on_lane0) |
                                    (eop[5] & !start_on_lane0) |
                                    (eop[6] & !start_on_lane0) |
                                    (eop[7] & !start_on_lane0);
 
                  // Add 8 bytes IFG
 
                  next_ifg_8b_add = (eop[0]) |
                                    (eop[1]) |
                                    (eop[2]) |
                                    (eop[3] & !start_on_lane0) |
                                    (eop[4]) |
                                    (eop[5]) |
                                    (eop[6]) |
                                    (eop[7]);
 
                  // Add another 8 bytes IFG
 
                  next_ifg_8b2_add = (eop[0] & !start_on_lane0) |
                                     (eop[1] & !start_on_lane0) |
                                     (eop[2] & !start_on_lane0);
 
              end
 
              if (|eop[2:0]) begin
 
                  if (frame_available) begin
 
                      // Next state depends on number of IFG bytes to be inserted.
                      // Skip idle state if needed.
 
                      if (next_ifg_8b2_add) begin
                          next_state_enc = SM_IFG;
                      end
                      else if (next_ifg_8b_add) begin
                          next_state_enc = SM_IDLE;
                      end
                      else begin
                          txhfifo_ren = 1'b1;
                          next_state_enc = SM_PREAMBLE;
                      end
 
                  end
                  else begin
                      next_state_enc = SM_IFG;
                  end
              end
 
              if (|eop[7:3]) begin
                  next_state_enc = SM_TERM;
              end
 
          end
 
        SM_TERM:
          begin
 
              // Insert TERMINATE character in correct lane depending on position
              // of EOP read from fifo. Also insert CRC read from control fifo.
 
              if (eop[3]) begin
                  next_xgxs_txd = {{7{`IDLE}}, `TERMINATE};
                  next_xgxs_txc = 8'b11111111;
              end
 
              if (eop[4]) begin
                  next_xgxs_txd = {{6{`IDLE}}, `TERMINATE, crc32_tx[31:24]};
                  next_xgxs_txc = 8'b11111110;
              end
 
              if (eop[5]) begin
                  next_xgxs_txd = {{5{`IDLE}}, `TERMINATE, crc32_tx[31:16]};
                  next_xgxs_txc = 8'b11111100;
              end
 
              if (eop[6]) begin
                  next_xgxs_txd = {{4{`IDLE}}, `TERMINATE, crc32_tx[31:8]};
                  next_xgxs_txc = 8'b11111000;
              end
 
              if (eop[7]) begin
                  next_xgxs_txd = {{3{`IDLE}}, `TERMINATE, crc32_tx[31:0]};
                  next_xgxs_txc = 8'b11110000;
              end
 
              // Next state depends on number of IFG bytes to be inserted.
              // Skip idle state if needed.
 
              if (frame_available && !ifg_8b_add) begin
                  txhfifo_ren = 1'b1;
                  next_state_enc = SM_PREAMBLE;
              end
              else if (frame_available) begin
                  next_state_enc = SM_IDLE;
              end
              else begin
                  next_state_enc = SM_IFG;
              end
 
          end
 
        SM_TERM_FAIL:
          begin
 
              next_xgxs_txd = {{7{`IDLE}}, `TERMINATE};
              next_xgxs_txc = 8'b11111111;
              next_state_enc = SM_IFG;
 
          end
 
        SM_IFG:
          begin
 
              next_state_enc = SM_IDLE;
 
          end
 
        default:
          begin
              next_state_enc = SM_IDLE;
          end
 
    endcase
 
end
 
 
always @(/*AS*/crc32_d64 or txhfifo_wen or txhfifo_wstatus) begin
 
    if (txhfifo_wen && txhfifo_wstatus[`TXSTATUS_SOP]) begin
        crc_data = 32'hffffffff;
    end
    else begin
        crc_data = crc32_d64;
    end
 
end
 
always @(/*AS*/byte_cnt or curr_state_pad or txdfifo_rdata
         or txdfifo_rempty or txdfifo_ren_d1 or txdfifo_rstatus
         or txhfifo_walmost_full) begin
 
    next_state_pad = curr_state_pad;
 
    next_txhfifo_wdata = txdfifo_rdata;
    next_txhfifo_wstatus = txdfifo_rstatus;
 
    txdfifo_ren = 1'b0;
    next_txhfifo_wen = 1'b0;
 
    case (curr_state_pad)
 
      SM_PAD_EQ: begin
 
 
          //---
          // If room availabe in hoding fifo and data available in
          // data fifo, transfer data words. If transmit state machine
          // is reading from fifo we can assume room will be available.
 
          if (!txhfifo_walmost_full) begin
 
              txdfifo_ren = !txdfifo_rempty;
 
          end
 
 
          //---
          // This logic dependent on read during previous cycle.
 
          if (txdfifo_ren_d1) begin
 
              next_txhfifo_wen = 1'b1;
 
              // On EOP, decide if padding is required for this packet.
 
              if (txdfifo_rstatus[`TXSTATUS_EOP]) begin
 
                  if (byte_cnt < 14'd56) begin
 
                      next_txhfifo_wstatus = `TXSTATUS_NONE;
                      txdfifo_ren = 1'b0;
                      next_state_pad = SM_PAD_PAD;
 
                  end
                  else if (byte_cnt == 14'd56 &&
                           (txdfifo_rstatus[2:0] == 3'd1 ||
                            txdfifo_rstatus[2:0] == 3'd2 ||
                            txdfifo_rstatus[2:0] == 3'd3)) begin
 
                      // Pad up to LANE3, keep the other 4 bytes for crc that will
                      // be inserted by dequeue engine.
 
                      next_txhfifo_wstatus[2:0] = 3'd4;
 
                      // Pad end bytes with zeros.
 
                      if (txdfifo_rstatus[2:0] == 3'd1)
                        next_txhfifo_wdata[31:8] = 24'b0;
                      if (txdfifo_rstatus[2:0] == 3'd2)
                        next_txhfifo_wdata[31:16] = 16'b0;
                      if (txdfifo_rstatus[2:0] == 3'd3)
                        next_txhfifo_wdata[31:24] = 8'b0;
 
                      txdfifo_ren = 1'b0;
 
                  end
                  else begin
 
                      txdfifo_ren = 1'b0;
 
                  end
 
              end
 
          end
 
      end
 
      SM_PAD_PAD: begin
 
          //---
          // Pad packet to 64 bytes by writting zeros to holding fifo.
 
          if (!txhfifo_walmost_full) begin
 
              next_txhfifo_wdata = 64'b0;
              next_txhfifo_wstatus = `TXSTATUS_NONE;
              next_txhfifo_wen = 1'b1;
 
              if (byte_cnt == 14'd56) begin
 
 
                  // Pad up to LANE3, keep the other 4 bytes for crc that will
                  // be inserted by dequeue engine.
 
                  next_txhfifo_wstatus[`TXSTATUS_EOP] = 1'b1;
                  next_txhfifo_wstatus[2:0] = 3'd4;
 
                  next_state_pad = SM_PAD_EQ;
 
              end
 
          end
 
      end
 
      default:
        begin
            next_state_pad = SM_PAD_EQ;
        end
 
    endcase
 
end
 
 
always @(posedge clk_xgmii_tx or negedge reset_xgmii_tx_n) begin
 
    if (reset_xgmii_tx_n == 1'b0) begin
 
        curr_state_pad <= SM_PAD_EQ;
 
        txdfifo_ren_d1 <= 1'b0;
 
        txhfifo_wdata <= 64'b0;
        txhfifo_wstatus <= 8'b0;
        txhfifo_wen <= 1'b0;   
 
        byte_cnt <= 14'b0;
 
        shift_crc_data <= 64'b0;
        shift_crc_eop <= 4'b0;
        shift_crc_cnt <= 4'b0;
 
    end
    else begin
 
        curr_state_pad <= next_state_pad;
 
        txdfifo_ren_d1 <= txdfifo_ren;
 
        txhfifo_wdata <= next_txhfifo_wdata;
        txhfifo_wstatus <= next_txhfifo_wstatus;
        txhfifo_wen <= next_txhfifo_wen;
 
 
        //---
        // Reset byte count on SOP
 
        if (next_txhfifo_wen) begin
 
            if (next_txhfifo_wstatus[`TXSTATUS_SOP]) begin
 
                byte_cnt <= 14'd8;
 
            end
            else begin
 
                byte_cnt <= byte_cnt + 14'd8;
 
            end
 
        end
 
 
        //---
        // Calculate CRC as data is written to holding fifo. The holding fifo creates
        // a delay that allow the CRC calculation to complete before the end of the frame
        // is ready to be transmited.
 
        if (txhfifo_wen) begin
 
            crc32_d64 <= nextCRC32_D64(reverse_64b(txhfifo_wdata), crc_data);
 
        end
 
        if (txhfifo_wen && txhfifo_wstatus[`TXSTATUS_EOP]) begin
 
            // Last bytes calculated 8-bit at a time instead of 64-bit. Start
            // this process at the end of the frame.
 
            crc32_d8 <= crc32_d64;
 
            shift_crc_data <= txhfifo_wdata;
            shift_crc_cnt <= 4'd9;
 
            if (txhfifo_wstatus[2:0] == 3'b0) begin
              shift_crc_eop <= 4'd8;
            end
            else begin
                shift_crc_eop <= {1'b0, txhfifo_wstatus[2:0]};
            end
 
        end
        else if (shift_crc_eop != 4'b0) begin
 
            // Complete crc calculation 8-bit at a time until finished. This can
            // be 1 to 8 bytes long.
 
            crc32_d8 <= nextCRC32_D8(reverse_8b(shift_crc_data[7:0]), crc32_d8);
 
            shift_crc_data <= {8'b0, shift_crc_data[63:8]};
            shift_crc_eop <= shift_crc_eop - 4'd1;
 
        end
 
 
        //---
        // Update CRC register at the end of calculation. Always update after 8
        // cycles for deterministic results, even if a single byte was present in
        // last data word.
 
        if (shift_crc_cnt == 4'b1) begin
 
            crc32_tx <= ~reverse_32b(crc32_d8);
 
        end
        else begin
 
            shift_crc_cnt <= shift_crc_cnt - 4'd1;
 
        end
 
    end
 
end
 
endmodule
 
