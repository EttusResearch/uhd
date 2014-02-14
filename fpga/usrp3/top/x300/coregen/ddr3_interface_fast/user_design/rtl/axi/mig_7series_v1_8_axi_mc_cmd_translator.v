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
// File name: axi_mc_cmd_translator.v
//
// Description: 
// INCR and WRAP burst modes are decoded in parallel and then the output is
// chosen based on the AxBURST value.  FIXED burst mode is not supported and
// is mapped to the INCR command instead.  
//
// Specifications:
//
///////////////////////////////////////////////////////////////////////////////
`timescale 1ps/1ps
`default_nettype none

module mig_7series_v1_8_axi_mc_cmd_translator #
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
                      // DRAM clock to AXI clock ratio
                    // supported values 2, 4
  parameter integer C_MC_nCK_PER_CLK             = 2, 
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
  input  wire [1:0]                           axburst       , 
  input  wire                                 axhandshake   , 
  output wire [C_MC_ADDR_WIDTH-1:0]           cmd_byte_addr , 
  output wire                                 ignore_begin  ,
  output wire                                 ignore_end    ,
  output wire                                 incr_burst    , 

  // Connections to/from fsm module
  // signal to increment to the next mc transaction 
  input  wire                                 next          , 
  // signal to the fsm there is another transaction required
  output wire                                 next_pending


);

////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
// AXBURST decodes
localparam P_AXBURST_FIXED = 2'b00;
localparam P_AXBURST_INCR  = 2'b01;
localparam P_AXBURST_WRAP  = 2'b10;

localparam P_MC_BURST_MASK = {C_MC_ADDR_WIDTH{1'b1}} ^ 
                             {C_MC_BURST_LEN+(C_MC_nCK_PER_CLK/2){1'b1}};
////////////////////////////////////////////////////////////////////////////////
// Wires/Reg declarations
////////////////////////////////////////////////////////////////////////////////
wire [C_AXI_ADDR_WIDTH-1:0]     cmd_byte_addr_i;

wire [C_AXI_ADDR_WIDTH-1:0]     axi_mc_incr_cmd_byte_addr;
wire                            incr_next_pending;
wire [C_AXI_ADDR_WIDTH-1:0]     axi_mc_wrap_cmd_byte_addr;
wire                            wrap_next_pending;
reg                             sel_first;
wire [1:0]                      ax_burst;
reg  [1:0]                      ax_burst_r;
wire                            incr_ignore_begin;
wire                            incr_ignore_end;
wire                            wrap_ignore_begin;
wire                            wrap_ignore_end;  
wire                            incr_incr_burst; 
reg                             axburst_eq1;
reg                             axburst_eq0;
reg                             sel_first_i;   

////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////

// INCR and WRAP translations are calcuated in independently, select the one
// for our transactions
// right shift by the UI width to the DRAM width ratio 
 
assign cmd_byte_addr    = (C_MC_nCK_PER_CLK == 4) ?
                          (cmd_byte_addr_i >> C_AXSIZE-3) & P_MC_BURST_MASK : 
                          (cmd_byte_addr_i >> C_AXSIZE-2) & P_MC_BURST_MASK;

assign cmd_byte_addr_i  = (ax_burst[1]) ? axi_mc_wrap_cmd_byte_addr 
                           : axi_mc_incr_cmd_byte_addr;


assign ignore_begin     = (ax_burst[1]) ? wrap_ignore_begin 
                          : incr_ignore_begin;

assign ignore_end       = (ax_burst[1]) ? wrap_ignore_end 
                          : incr_ignore_end;
assign incr_burst       = (ax_burst[1]) ? 1'b0 :
                            incr_incr_burst;

// Indicates if we are on the first transaction of a mc translation with more
// than 1 transaction.
always @(posedge clk) begin
  if (reset | axhandshake) begin
    sel_first <= 1'b1;
  end else if (next) begin
    sel_first <= 1'b0;
  end
end

assign ax_burst = (sel_first) ? axburst : ax_burst_r;

always @(posedge clk) begin
   ax_burst_r <= ax_burst;
end



always @(*) begin
  if (reset | axhandshake) begin
    sel_first_i = 1'b1;
  end else if (next) begin
    sel_first_i = 1'b0;
  end else begin
    sel_first_i = sel_first;
  end
end

assign next_pending = axburst[1]? axburst_eq1 : axburst_eq0;

always @(posedge clk) begin
  axburst_eq1 <= (sel_first_i? 1'b1 : ax_burst[1])? wrap_next_pending : incr_next_pending;
  axburst_eq0 <= (sel_first_i? 1'b0 : ax_burst[1])? wrap_next_pending : incr_next_pending;
end

mig_7series_v1_8_axi_mc_incr_cmd #
(
  .C_AXI_ADDR_WIDTH  (C_AXI_ADDR_WIDTH),
  .C_MC_ADDR_WIDTH  (C_MC_ADDR_WIDTH),
  .C_DATA_WIDTH     (C_DATA_WIDTH),
  .C_MC_BURST_LEN   (C_MC_BURST_LEN),
  .C_AXSIZE         (C_AXSIZE)
)
axi_mc_incr_cmd_0
(
  .clk           ( clk                ) ,
  .reset         ( reset              ) ,
  .axaddr        ( axaddr             ) ,
  .axlen         ( axlen              ) ,
  .axsize        ( axsize             ) ,
  .axhandshake   ( axhandshake        ) ,
  .cmd_byte_addr ( axi_mc_incr_cmd_byte_addr ) ,
  .ignore_begin  ( incr_ignore_begin  ) ,
  .ignore_end    ( incr_ignore_end    ) ,
  .incr_burst    ( incr_incr_burst    ) ,
  .next          ( next               ) ,
  .next_pending  ( incr_next_pending  ) 
);

mig_7series_v1_8_axi_mc_wrap_cmd #
(
  .C_AXI_ADDR_WIDTH (C_AXI_ADDR_WIDTH),
  .C_MC_ADDR_WIDTH  (C_MC_ADDR_WIDTH),
  .C_MC_BURST_LEN   (C_MC_BURST_LEN),
  .C_DATA_WIDTH     (C_DATA_WIDTH),
  .C_AXSIZE         (C_AXSIZE)
)
axi_mc_wrap_cmd_0
(
  .clk           ( clk                ) ,
  .reset         ( reset              ) ,
  .axaddr        ( axaddr             ) ,
  .axlen         ( axlen              ) ,
  .axsize        ( axsize             ) ,
  .axhandshake   ( axhandshake        ) ,
  .ignore_begin  ( wrap_ignore_begin  ) ,
  .ignore_end    ( wrap_ignore_end    ) ,
  .cmd_byte_addr ( axi_mc_wrap_cmd_byte_addr ) ,
  .next          ( next               ) ,
  .next_pending  ( wrap_next_pending  ) 
);

endmodule
`default_nettype wire
