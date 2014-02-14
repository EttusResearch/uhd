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
// File name: axi_mc_wrap_cmd.v
//
// Description: 
// MC does not support an AXI WRAP command directly.  
// To complete an AXI WRAP transaction we will issue one transaction if the
// address is wrap boundary aligned, otherwise two transactions are issued.
// The first transaction is from the starting offset to the wrap address upper
// boundary.  The second transaction is from the wrap boundary lowest address
// to the address offset. WRAP burst types will never exceed 16 beats.
//
// Calculates the number of MC beats for each axi transaction for WRAP
// burst type ( for all axsize values = C_DATA_WIDTH ):
// AR_SIZE   | AR_LEN     | OFFSET | NUM_BEATS 1 | NUM_BEATS 2
// b010(  4) | b0001(  2) |  b0000 |   2         |   0 
// b010(  4) | b0001(  2) |  b0001 |   1         |   1 
// b010(  4) | b0011(  4) |  b0000 |   4         |   0 
// b010(  4) | b0011(  4) |  b0001 |   3         |   1 
// b010(  4) | b0011(  4) |  b0010 |   2         |   2 
// b010(  4) | b0011(  4) |  b0011 |   1         |   3 
// b010(  4) | b0111(  8) |  b0000 |   8         |   0 
// b010(  4) | b0111(  8) |  b0001 |   7         |   1 
// b010(  4) | b0111(  8) |  b0010 |   6         |   2 
// b010(  4) | b0111(  8) |  b0011 |   5         |   3 
// b010(  4) | b0111(  8) |  b0100 |   4         |   4 
// b010(  4) | b0111(  8) |  b0101 |   3         |   5 
// b010(  4) | b0111(  8) |  b0110 |   2         |   6 
// b010(  4) | b0111(  8) |  b0111 |   1         |   7 
// b010(  4) | b1111( 16) |  b0000 |  16         |   0 
// b010(  4) | b1111( 16) |  b0001 |  15         |   1 
// b010(  4) | b1111( 16) |  b0010 |  14         |   2 
// b010(  4) | b1111( 16) |  b0011 |  13         |   3 
// b010(  4) | b1111( 16) |  b0100 |  12         |   4 
// b010(  4) | b1111( 16) |  b0101 |  11         |   5 
// b010(  4) | b1111( 16) |  b0110 |  10         |   6 
// b010(  4) | b1111( 16) |  b0111 |   9         |   7 
// b010(  4) | b1111( 16) |  b1000 |   8         |   8 
// b010(  4) | b1111( 16) |  b1001 |   7         |   9 
// b010(  4) | b1111( 16) |  b1010 |   6         |  10 
// b010(  4) | b1111( 16) |  b1011 |   5         |  11 
// b010(  4) | b1111( 16) |  b1100 |   4         |  12 
// b010(  4) | b1111( 16) |  b1101 |   3         |  13 
// b010(  4) | b1111( 16) |  b1110 |   2         |  14 
// b010(  4) | b1111( 16) |  b1111 |   1         |  15 
///////////////////////////////////////////////////////////////////////////////

`timescale 1ps/1ps
`default_nettype none

module mig_7series_v1_8_axi_mc_wrap_cmd #
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
                    // MC burst length. = 1 for BL4 or BC4, = 2 for BL8
  parameter integer C_MC_BURST_LEN              = 1,
                    // Width of AXI xDATA and MC xx_data
                    // Range: 32, 64, 128.
  parameter integer C_DATA_WIDTH                = 32,
  // Static value of axsize
                    // Range: 2-5
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
  output wire                                 ignore_begin  ,
  output wire                                 ignore_end    ,
  output wire [C_AXI_ADDR_WIDTH-1:0]          cmd_byte_addr , 

  // Connections to/from fsm module
  // signal to increment to the next mc transaction 
  input  wire                                 next          , 
  // signal to the fsm there is another transaction required
  output reg                                  next_pending 

);
////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
localparam P_AXLEN_WIDTH = 4;
////////////////////////////////////////////////////////////////////////////////
// Wire and register declarations
////////////////////////////////////////////////////////////////////////////////
reg                         sel_first;
wire [C_AXI_ADDR_WIDTH-1:0] axaddr_i;
wire [3:0]                  axlen_i;
reg  [C_AXI_ADDR_WIDTH-1:0] wrap_boundary_axaddr;
reg  [3:0]                  axaddr_offset;
reg  [3:0]                  wrap_first_len;
reg  [3:0]                  wrap_second_len;
reg  [C_AXI_ADDR_WIDTH-1:0] wrap_boundary_axaddr_r;
reg  [3:0]                  axaddr_offset_r;
reg  [3:0]                  wrap_first_len_r;
reg  [3:0]                  wrap_second_len_r;
reg  [4:0]                  axlen_cnt;
reg  [4:0]                  wrap_cnt_r;
wire [4:0] 		    wrap_cnt;
reg  [C_AXI_ADDR_WIDTH-1:0] axaddr_wrap;
wire                        first_odd;
wire                        second_odd;
wire                        first_offset;
wire                        second_offset;
wire [1:0]                  first_extra;
wire [2:0]                  second_extra; 
reg                         first_odd_r;
reg                         second_odd_r;
reg                         first_offset_r;
reg                         second_offset_r;
reg [1:0]                   first_extra_r;
reg [2:0]                   second_extra_r;   
wire [7:0]                  incr_cnt;
wire                        ignore_begin_cond;
wire                        ignore_end_cond;
wire                        ignore_begin_first;
wire                        ignore_end_first;
wire                        ignore_begin_second;
wire                        ignore_end_second;
reg                         next_pending_r;

////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////
assign cmd_byte_addr = (sel_first) ? axaddr: axaddr_wrap;
assign axaddr_i = axaddr[0 +: C_AXI_ADDR_WIDTH];
assign axlen_i = axlen[3:0];
// Mask bits based on transaction length to get wrap boundary low address
always @(*) begin
  if(axhandshake) 
    wrap_boundary_axaddr = axaddr_i & ~(axlen_i << axsize);
  else
    wrap_boundary_axaddr = wrap_boundary_axaddr_r;
end

// Offset used to calculate the length of each transaction
always @( *) begin
  if(axhandshake)
    axaddr_offset = axaddr_i[axsize +: 4] & axlen_i;
  else
    axaddr_offset = axaddr_offset_r; 
end
   
// The first and the second command from the wrap transaction could
// be of odd length or even length with address offset. This will be 
// an issue with BL8, extra transactions have to be issued.
// Rounding up the length to account for extra transactions. 
always @(*) begin
  if(axhandshake) begin
    wrap_first_len = (C_MC_BURST_LEN == 1) ? (axlen_i - axaddr_offset) :
                        (axlen_i - axaddr_offset ) + axaddr_offset[0]; 
    wrap_second_len = (C_MC_BURST_LEN == 1) ? 
              (axaddr_offset >0) ? axaddr_offset - 1 : 0:
              (axaddr_offset >0)? (axaddr_offset-1) + axaddr_offset[0]: 0;
  end else begin
    wrap_first_len = wrap_first_len_r;
    wrap_second_len = wrap_second_len_r;
  end
end

// registering to be used in the combo logic. 
always @(posedge clk) begin
  wrap_boundary_axaddr_r <= wrap_boundary_axaddr;
  axaddr_offset_r <= axaddr_offset;
  wrap_first_len_r <= wrap_first_len;
  wrap_second_len_r <= wrap_second_len;
  first_odd_r <= first_odd;
  second_odd_r <= second_odd;
  first_offset_r <= first_offset;
  second_offset_r <= second_offset;
  first_extra_r <= first_extra;
  second_extra_r <= second_extra; 
end
   
// Determining if the first and second transations are of odd len
assign first_odd = ((axlen_i - axaddr_offset) +1) % C_MC_BURST_LEN;
assign second_odd = axaddr_offset % C_MC_BURST_LEN;


// Figuring out if the address have an offset for padding data in BL8 case
assign first_offset = (C_MC_BURST_LEN == 1) ? 1'b0 : axaddr_i[C_AXSIZE];
assign second_offset = (C_MC_BURST_LEN == 1) ? 1'b0 : 
                        wrap_boundary_axaddr[C_AXSIZE];

// determining if extra data is required for even offsets
assign first_extra = (first_odd) ? 2'b01 : {1'b0, (first_offset*2)};
assign second_extra = (second_odd) ? {2'b00, ~first_odd}:
      {2'b00, (second_offset*2)}+ {2'b00, (~first_odd &(|axaddr_offset))};   

// wrap_cnt used to switch the address for first and second transaction.
assign wrap_cnt = (C_MC_BURST_LEN == 2) ? (axaddr_offset_r >0) ?
                          {1'd0, wrap_second_len + second_extra + 1 }:
                          {1'd0, wrap_second_len + second_extra} :
                          {1'd0, wrap_second_len + second_extra}; 

always @(posedge clk)
  wrap_cnt_r <= wrap_cnt;

always @(posedge clk) begin
  if(next)begin
    if((axlen_cnt == wrap_cnt_r) ||
      (axlen_cnt == wrap_cnt_r + C_MC_BURST_LEN/2))
      axaddr_wrap <= wrap_boundary_axaddr_r;
    else
      axaddr_wrap <= axaddr_wrap + (incr_cnt * C_MC_BURST_LEN);
  end else if (sel_first) 
      axaddr_wrap <= axaddr;
end 


// figuring out how much to much to incr based on AXSIZE
assign incr_cnt = (C_AXSIZE == 2) ? 8'd4 : (C_AXSIZE == 3) ? 8'd8 :
       (C_AXSIZE == 4)? 8'd16 :(C_AXSIZE == 5) ? 8'd32 : 
       (C_AXSIZE == 6) ? 8'd64 :  (C_AXSIZE == 7) ? 8'd128 :8'd0;

// Even numbber of transactions with offset, inc len by 2 for BL8
always @(posedge clk) begin
  if (axhandshake)begin
    axlen_cnt <= wrap_first_len + wrap_second_len 
                + first_extra + second_extra;
    next_pending_r <= (wrap_first_len + wrap_second_len 
                + first_extra + second_extra) >= C_MC_BURST_LEN;
  end else if (next) begin
    if (axlen_cnt > C_MC_BURST_LEN) begin
      axlen_cnt <= axlen_cnt - C_MC_BURST_LEN;
      next_pending_r <= (axlen_cnt - C_MC_BURST_LEN) >= C_MC_BURST_LEN;
    end else begin
      axlen_cnt <= 5'd0;
      next_pending_r <= 1'b0;
    end
  end  
end  

always @(*) begin
  if (axhandshake)begin
    next_pending = (wrap_first_len + wrap_second_len 
                + first_extra + second_extra) >= C_MC_BURST_LEN;
  end else if (next) begin
    if (axlen_cnt > C_MC_BURST_LEN) begin
      next_pending = (axlen_cnt - C_MC_BURST_LEN) >= C_MC_BURST_LEN;
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
 
assign ignore_begin_cond = ignore_begin_first | ignore_begin_second;  

assign ignore_end = (C_MC_BURST_LEN == 1) ? 1'b0 : ignore_end_cond;
   
assign ignore_end_cond = ignore_end_first | ignore_end_second;

// ignore logic for first transaction        
assign ignore_begin_first = ((axlen_cnt == (wrap_first_len_r + 
             wrap_second_len_r + first_extra_r + second_extra_r)) &
             (first_offset_r));

assign ignore_end_first = ((axlen_cnt == (wrap_second_len_r + second_extra_r
                     + 1 +first_extra_r)) &
          ((first_odd_r & ~first_offset_r) | (~first_odd_r & first_offset_r)));

// ignore logic for second transaction.    
assign ignore_begin_second = ((axlen_cnt == (wrap_second_len_r + second_extra_r))&
                              (second_offset_r));
   
assign ignore_end_second  = ((axlen_cnt < C_MC_BURST_LEN) &
      ((second_odd_r & ~second_offset_r ) | (~second_odd_r & second_offset_r)));




 
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
