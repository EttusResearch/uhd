// -- (c) Copyright 2010 Xilinx, Inc. All rights reserved. 
// --                                                             
// -- This file contains confidential and proprietary information 
// -- of Xilinx, Inc. and is protected under U.S. and             
// -- international copyright and other intellectual property     
// -- laws.                                                       
// --                                                             
// -- DISCLAIMER                                                  
// -- This disclaimer is not a license and does not grant any     
// -- rights to the materials distributed herewith. Except as     
// -- otherwise provided in a valid license issued to you by      
// -- Xilinx, and to the maximum extent permitted by applicable   
// -- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND     
// -- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES 
// -- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING   
// -- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-      
// -- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and    
// -- (2) Xilinx shall not be liable (whether in contract or tort,
// -- including negligence, or under any other theory of          
// -- liability) for any loss or damage of any kind or nature     
// -- related to, arising under or in connection with these       
// -- materials, including for any direct, or any indirect,       
// -- special, incidental, or consequential loss or damage        
// -- (including loss of data, profits, goodwill, or any type of  
// -- loss or damage suffered as a result of any action brought   
// -- by a third party) even if such damage or loss was           
// -- reasonably foreseeable or Xilinx had been advised of the    
// -- possibility of the same.                                    
// --                                                             
// -- CRITICAL APPLICATIONS                                       
// -- Xilinx products are not designed or intended to be fail-    
// -- safe, or for use in any application requiring fail-safe     
// -- performance, such as life-support or safety devices or      
// -- systems, Class III medical devices, nuclear facilities,     
// -- applications related to the deployment of airbags, or any   
// -- other applications that could lead to death, personal       
// -- injury, or severe property or environmental damage          
// -- (individually and collectively, "Critical                   
// -- Applications"). Customer assumes the sole risk and          
// -- liability of any use of Xilinx products in Critical         
// -- Applications, subject only to applicable laws and           
// -- regulations governing limitations on product liability.     
// --                                                             
// -- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS    
// -- PART OF THIS FILE AT ALL TIMES.                             
// --  
///////////////////////////////////////////////////////////////////////////////
//
// File name: axi_mc_incr_cmd.v
//
// Description: 
// MC does not support up to 256 beats per transaction to support an AXI INCR 
// command directly.  Additionally for QOS purposes, larger transactions
// issued as many smaller transactions should improve QoS for the system.
// In the BL8 mode depending on the address offset ragged head or ragged tail
// need to be inserted into the data stream for writes and ignored for reads.
// In BL8 mode for transactions with odd length and even length transactions
// with an address offset an extra BL8 transaction will be issued. 
///////////////////////////////////////////////////////////////////////////////

`timescale 1ps/1ps
`default_nettype none

module mig_7series_v1_8_axi_mc_incr_cmd #
(
///////////////////////////////////////////////////////////////////////////////
// Parameter Definitions
///////////////////////////////////////////////////////////////////////////////
                    // Width of AxADDR
                    // Range: 32.
  parameter integer C_AXI_ADDR_WIDTH            = 32, 
                    // Width of cmd_byte_addr
                    // Range: 30
  parameter integer C_MC_ADDR_WIDTH             = 30,
                    // Width of AXI xDATA and MC xx_data
                    // Range: 32, 64, 128.
  parameter integer C_DATA_WIDTH                = 32,
                    // MC burst length. = 1 for BL4 or BC4, = 2 for BL8
  parameter integer C_MC_BURST_LEN              = 1,
                    // Static value of axsize
                    // Range: 2-4
  parameter integer C_AXSIZE                    = 2
)
(
///////////////////////////////////////////////////////////////////////////////
// Port Declarations     
///////////////////////////////////////////////////////////////////////////////
  input  wire                                 clk           , 
  input  wire                                 reset         , 
  input  wire [C_AXI_ADDR_WIDTH-1:0]          axaddr        , 
  input  wire [7:0]                           axlen         , 
  input  wire [2:0]                           axsize        , 
  // axhandshake = axvalid & axready
  input  wire                                 axhandshake   , 
  output wire [C_AXI_ADDR_WIDTH-1:0]          cmd_byte_addr ,
  output wire                                 ignore_begin  ,
  output wire                                 ignore_end    ,
  output wire                                 incr_burst    ,
  // Connections to/from fsm module
  // signal to increment to the next mc transaction 
  input  wire                                 next          , 
  // signal to the fsm there is another transaction required
  output reg                                  next_pending 

);
////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
localparam P_AXLEN_WIDTH = 8;    


////////////////////////////////////////////////////////////////////////////////
// Wire and register declarations
////////////////////////////////////////////////////////////////////////////////
reg                           sel_first;              
reg  [C_AXI_ADDR_WIDTH-1:0]   axaddr_incr;          
reg  [8:0]                    axlen_cnt;     
wire                          addr_offset;
reg                           addr_offset_r;
reg                           length_even_r;
wire                          length_even;
wire [7:0]                    incr_cnt;
wire                          ignore_begin_cond;
wire                          ignore_end_cond;
reg                           next_pending_r;
reg [7:0] 		      axlen_r;

   
   


////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////

// figuring out how much to much to incr based on AXSIZE
assign incr_cnt = (C_AXSIZE == 2) ? 8'd4 : (C_AXSIZE == 3) ? 8'd8 :
       (C_AXSIZE == 4)? 8'd16 :(C_AXSIZE == 5) ? 8'd32 : 
       (C_AXSIZE == 6) ? 8'd64 :  (C_AXSIZE == 7) ? 8'd128 :8'd0;
   
   
// calculate cmd_byte_addr
assign cmd_byte_addr = (sel_first) ? axaddr : axaddr_incr;

// Incremented version of axaddr
always @(posedge clk) begin
  if (sel_first) begin
    if(~next)
      axaddr_incr <= axaddr;
    else
      axaddr_incr <= axaddr + (incr_cnt * C_MC_BURST_LEN); 
  end else if (next) begin
    axaddr_incr <= axaddr_incr + (incr_cnt * C_MC_BURST_LEN);     
  end 
end

// The start address could have an offset
// determining if the address has offset.
assign addr_offset = axaddr[C_AXSIZE] ;

// The length could be odd which is an issue in BL8
assign length_even = axlen[0];

// Register for timing
always @(posedge clk) begin
  addr_offset_r <= addr_offset;
  length_even_r <= length_even;
  axlen_r <= axlen;
end 
   

// In BL8 for even length transactions with offset and for odd len
//transactions the len  has to be increased by 1 to issue an extra transaction. 
always @(posedge clk) begin
  if (axhandshake)begin
     if((C_MC_BURST_LEN==2) &&
        (addr_offset & length_even)) begin
       axlen_cnt <= axlen + 1;
       next_pending_r <= ((axlen + 1) >= C_MC_BURST_LEN);
     end else begin 
       axlen_cnt <= axlen;
       next_pending_r <= (axlen >= C_MC_BURST_LEN);
     end
  end else if (next) begin
    if (axlen_cnt > C_MC_BURST_LEN) begin
      axlen_cnt <= axlen_cnt - C_MC_BURST_LEN;
      next_pending_r <= ((axlen_cnt - C_MC_BURST_LEN) >= C_MC_BURST_LEN);
    end else begin
      axlen_cnt <= 9'd0;
      next_pending_r <= 1'b0;
    end
  end  
end  

always @(*) begin
  if (axhandshake)begin
     if((C_MC_BURST_LEN==2) &&
        (addr_offset & length_even)) begin
       next_pending = ((axlen + 1) >= C_MC_BURST_LEN);
     end else begin 
       next_pending = (axlen >= C_MC_BURST_LEN);
     end
  end else if (next) begin
    if (axlen_cnt > C_MC_BURST_LEN) begin
      next_pending = ((axlen_cnt - C_MC_BURST_LEN) >= C_MC_BURST_LEN);
    end else begin
      next_pending = 1'b0;
    end
  end else begin
    next_pending = next_pending_r;
  end
end  

// last and ignore signals to data channel. These signals are used for
// BL8 to ignore and insert data for even len transactions with offset
// and odd len transactions
// For odd len transactions with no offset the last read is ignored and
// last write is masked
// For odd len transactions with offset the first read is ignored and
// first write is masked
// For even len transactions with offset the last & first read is ignored and
// last& first  write is masked
// For even len transactions no ingnores or masks. 

assign ignore_begin = (C_MC_BURST_LEN == 1) ? 1'b0 : ignore_begin_cond;

assign ignore_begin_cond =  addr_offset_r && (axlen_cnt == (axlen_r +  
                            (addr_offset_r & length_even_r)));

assign ignore_end = (C_MC_BURST_LEN == 1) ? 1'b0 : ignore_end_cond;
   
assign ignore_end_cond = (((~length_even_r & ~addr_offset_r) | 
                           (length_even_r & addr_offset_r)) & (axlen_cnt == 0));

assign incr_burst = (C_MC_BURST_LEN == 1) ? 1'b1: 
                     (axlen_cnt > C_MC_BURST_LEN);
   
// Indicates if we are on the first transaction of a mc translation with more
// than 1 transaction.
always @(posedge clk) begin
  if (reset | axhandshake) begin
    sel_first <= 1'b1;
  end else if (next) begin
    sel_first <= 1'b0;
  end
end

endmodule
`default_nettype wire
