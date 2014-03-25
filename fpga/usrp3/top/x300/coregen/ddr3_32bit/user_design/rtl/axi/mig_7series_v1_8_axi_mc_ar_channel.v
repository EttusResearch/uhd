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
// File name: axi_mc_ar_channel.v
//
// Description: 
//
///////////////////////////////////////////////////////////////////////////////
`timescale 1ps/1ps
`default_nettype none

module mig_7series_v1_8_axi_mc_ar_channel #
(
///////////////////////////////////////////////////////////////////////////////
// Parameter Definitions
///////////////////////////////////////////////////////////////////////////////
                    // Width of ID signals.
                    // Range: >= 1.
  parameter integer C_ID_WIDTH          = 4, 
                    // Width of AxADDR
                    // Range: 32.
  parameter integer C_AXI_ADDR_WIDTH    = 32, 
                    // Width of cmd_byte_addr
                    // Range: 30
  parameter integer C_MC_ADDR_WIDTH     = 30,
                    // Width of AXI xDATA and MC  xx_data
                    // Range: 32, 64, 128.
  parameter integer C_DATA_WIDTH        = 32,
                        // MC burst length. = 1 for BL4 or BC4, = 2 for BL8
  parameter integer C_MC_BURST_LEN              = 1,
                    // DRAM clock to AXI clock ratio
                    // supported values 2, 4
  parameter integer C_MC_nCK_PER_CLK             = 2, 
                    // Static value of axsize
                    // Range: 2-4
  parameter integer C_AXSIZE            = 2
  
)
(
///////////////////////////////////////////////////////////////////////////////
// Port Declarations     
///////////////////////////////////////////////////////////////////////////////
  // AXI Slave Interface
  // Slave Interface System Signals           
  input  wire                                 clk             , 
  input  wire                                 reset           , 

  // Slave Interface Read Address Ports
  input  wire [C_ID_WIDTH-1:0]                arid            , 
  input  wire [C_AXI_ADDR_WIDTH-1:0]          araddr          , 
  input  wire [7:0]                           arlen           , 
  input  wire [2:0]                           arsize          , 
  input  wire [1:0]                           arburst         , 
  input  wire [1:0]                           arlock          , 
  input  wire [3:0]                           arcache         , 
  input  wire [2:0]                           arprot          , 
  input  wire [3:0]                           arqos           , 
  input  wire                                 arvalid         , 
  output wire                                 arready         , 

  // MC  Master Interface
  // Status
  input  wire                                 mc_init_complete , 
  //CMD PORT
  output wire                                 cmd_en           , 
  output wire [2:0]                           cmd_instr        , 
  output wire [C_MC_ADDR_WIDTH-1:0]           cmd_byte_addr    ,  
  input  wire                                 cmd_full         ,
  output wire                                 read_last        ,

  // Connections to/from axi_mc_r_channel module
  output wire                                 r_push           , 
  output wire [C_ID_WIDTH-1:0]                r_arid           , 
  output wire                                 r_rlast          ,
  output wire                                 r_ignore_begin   ,
  output wire                                 r_ignore_end     ,
  input  wire                                 r_full        

);

////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
localparam                          P_CMD_WRITE                = 3'b000;
localparam                          P_CMD_READ                 = 3'b001;

////////////////////////////////////////////////////////////////////////////////
// Wires/Reg declarations
////////////////////////////////////////////////////////////////////////////////
wire                        next      ;
wire                        next_pending ;
wire                        b_push;
wire                        a_push;
wire                        w_push;
wire                        incr_burst;   
reg [C_ID_WIDTH-1:0]        arid_r; 
   

////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////


// Translate the AXI transaction to the MC transaction(s)
mig_7series_v1_8_axi_mc_cmd_translator #
(
  .C_AXI_ADDR_WIDTH ( C_AXI_ADDR_WIDTH ) ,
  .C_MC_ADDR_WIDTH  ( C_MC_ADDR_WIDTH  ) ,
  .C_DATA_WIDTH     ( C_DATA_WIDTH     ) ,
  .C_MC_BURST_LEN   ( C_MC_BURST_LEN   ) ,
  .C_MC_nCK_PER_CLK ( C_MC_nCK_PER_CLK ) ,
  .C_AXSIZE         ( C_AXSIZE         ) 
)
axi_mc_cmd_translator_0
(
  .clk           ( clk                   ) ,
  .reset         ( reset                 ) ,
  .axaddr        ( araddr                ) ,
  .axlen         ( arlen                 ) ,
  .axsize        ( arsize                ) ,
  .axburst       ( arburst               ) ,
  .axhandshake   ( arvalid & a_push      ) ,
  .ignore_begin  ( r_ignore_begin        ) ,
  .ignore_end    ( r_ignore_end          ) ,
  .incr_burst    ( incr_burst            ) ,
  .cmd_byte_addr ( cmd_byte_addr         ) ,
  .next          ( next                  ) ,
  .next_pending  ( next_pending          ) 
);

mig_7series_v1_8_axi_mc_cmd_fsm #
(
 .C_MC_BURST_LEN   ( C_MC_BURST_LEN   ),
 .C_MC_RD_INST     ( 1                )
)
ar_cmd_fsm_0
(
  .clk          ( clk            ) ,
  .reset        ( reset          ) ,
  .axready      ( arready        ) ,
  .axvalid      ( arvalid        ) ,
  .axlen        ( arlen          ) ,
  .cmd_en       ( cmd_en         ) ,
  .cmd_full     ( cmd_full       ) ,
  .next         ( next           ) ,
  .next_pending ( next_pending   ) ,
  .data_ready   ( ~r_full        ) ,
  .incr_burst   ( incr_burst     ) ,
  .init_complete(mc_init_complete) ,
  .b_push       ( b_push         ) ,
  .b_full       ( 1'b0           ) ,
  .a_push       ( a_push         ) ,
  .w_push       ( w_push         ) ,
  .r_push       ( r_push         ) ,
  .last_tran    ( read_last      )
);
   
assign cmd_instr = P_CMD_READ;
// these signals can be moved out of this block to the top level. 
assign r_arid  = arid_r;
assign r_rlast = ~next_pending;

always @(posedge clk) begin
  arid_r <= arid ;
end   
   
endmodule

`default_nettype wire
