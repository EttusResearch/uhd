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
// File name: axi_mc_wr_cmd_fsm.v
//
// Description: 
// Simple state machine to handle sending commands from AXI to MC.  The flow:
// 1. A transaction can only be initiaited when axvalid is true and data_ready
// is true.  For writes, data_ready means that  one completed BL8 or BL4 write 
// data has been pushed into the MC write FIFOs.  For read operations,
// data_ready indicates that there is enough room to push the transaction into
// the read FIF & read transaction fifo in the shim.  If the FIFO's in the 
// read channel module is full, then the state machine waits for the 
// FIFO's to drain out. 
//
// 2. When CMD_EN is asserted, it remains high until we sample CMD_FULL in
// a low state.  When CMD_EN == 1'b1, and CMD_FULL == 1'b0, then the command
// has been accepted.  When the command is accepted, if the next_pending
// signal is high we will incremented to the next transaction and issue the
// cmd_en again when data_ready is high.  Otherwise we will go to the done
// state.
//
// 3. The AXI transaction can only complete when b_full is not true (for writes)
// and no more mc transactions need to be issued.  The AXREADY will be
// asserted and the state machine will progress back to the the IDLE state.
// 
///////////////////////////////////////////////////////////////////////////////
`timescale 1ps/1ps
`default_nettype none

module mig_7series_v1_8_axi_mc_wr_cmd_fsm #(
///////////////////////////////////////////////////////////////////////////////
// Parameter Definitions
///////////////////////////////////////////////////////////////////////////////
                        // MC burst length. = 1 for BL4 or BC4, = 2 for BL8
  parameter integer C_MC_BURST_LEN              = 1,
                     // parameter to identify rd or wr instantation
                     // = 1 rd , = 0 wr 
  parameter integer C_MC_RD_INST              = 0
  
)
(
///////////////////////////////////////////////////////////////////////////////
// Port Declarations     
///////////////////////////////////////////////////////////////////////////////
  input  wire                                 clk           , 
  input  wire                                 reset         , 
  output wire                                 axready       , 
  input  wire                                 axvalid       , 
  output wire                                 cmd_en        , 
  input  wire                                 cmd_full      , 
  // signal to increment to the next mc transaction 
  output wire                                 next          , 
  // signal to the fsm there is another transaction required
  input  wire                                 next_pending  ,
  // signal to the fsm indicating incr transcation is happening
  input  wire                                 incr_burst    ,
  // Write Data portion has completed or Read FIFO has a slot available (not
  // full)
  input  wire                                 data_ready    ,
  // calibration complete signal from MC 
  input  wire                                 init_complete ,
  // status signal for w_channel when command is written. 
  output wire                                 b_push        ,
  input  wire                                 b_full        ,
  output wire                                 a_push        ,
  output wire                                 w_push        , 
  output wire                                 r_push        ,
  output wire                                 last_tran   
);

////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
// States
// localparam SM_IDLE                = 3'b001;
// localparam SM_CMD_EN              = 3'b010;
// localparam SM_CMD_ACCEPTED        = 3'b011;
// localparam SM_DONE_WAIT           = 3'b100;
// localparam SM_DONE                = 3'b101;
// localparam SM_DATA_WAIT           = 3'b110;
// localparam SM_FAIL                = 3'b111;
localparam SM_IDLE                = 2'b00;
localparam SM_CMD_EN              = 2'b01;
localparam SM_CMD_ACCEPTED        = 2'b10;
localparam SM_DONE_WAIT           = 2'b11;

////////////////////////////////////////////////////////////////////////////////
// Wires/Reg declarations
////////////////////////////////////////////////////////////////////////////////
reg [1:0]       state;
// synthesis attribute MAX_FANOUT of state is 20;
reg [1:0]       state_r1;
reg [1:0]       next_state;
reg             init_complete_r;
reg             init_complete_r1;
reg             next_r;      

////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
///////////////////////////////////////////////////////////////////////////////


// register for timing
always @(posedge clk) begin
  init_complete_r  <= init_complete;
  init_complete_r1 <= init_complete_r;
end 

always @(posedge clk) begin
  if (reset || (~init_complete_r1)) begin
    state <= SM_IDLE;
    state_r1 <= SM_IDLE;
  end else begin
    state <= next_state;
    state_r1 <= state;
  end
end



generate
  if (C_MC_BURST_LEN == 1) begin : gen_bc1
  
    // Next state transitions.
    always @(*)
    begin
      next_state = state;
      case (state)
        SM_IDLE:
          if (axvalid & data_ready)
            next_state = SM_CMD_EN;
          else
            next_state = state;
      
        SM_CMD_EN:
          if (~cmd_full & next_pending & incr_burst)
            next_state = SM_CMD_EN;
          else if (~cmd_full & next_pending)
            next_state = SM_CMD_ACCEPTED;
          else if (~cmd_full & ~next_pending & b_full)
            next_state = SM_DONE_WAIT;
          else if (~cmd_full & ~next_pending & ~b_full)
            next_state = SM_IDLE;
          else
            next_state = state;            

        SM_CMD_ACCEPTED:
          if (data_ready )
            next_state = SM_CMD_EN;
          else 
            next_state = state;

        SM_DONE_WAIT:
          if (!b_full)
            next_state = SM_IDLE;
          else 
            next_state = state;

          default:
            next_state = SM_IDLE;
      endcase
    end

    // Assign outputs based on current state.

    assign cmd_en  = ((state == SM_CMD_EN)  & (( ~incr_burst | data_ready ) | 
                     (~next_pending)));
                 
    assign next    = (((state == SM_CMD_ACCEPTED)  & (state_r1 != SM_CMD_ACCEPTED))
                     | ((state == SM_CMD_EN) & (~cmd_full) & (incr_burst) & (data_ready)) 
		     | (((state == SM_CMD_EN) | (state == SM_DONE_WAIT)) & (next_state == SM_IDLE))) ;

    assign 	   r_push  = next;

    assign a_push  = (state == SM_IDLE);
    assign axready = ((state == SM_CMD_EN) | (state == SM_DONE_WAIT)) & (next_state == SM_IDLE);
    assign b_push  = ((state == SM_CMD_EN) | (state == SM_DONE_WAIT)) & (next_state == SM_IDLE);
    assign last_tran = (state == SM_CMD_EN) & (~next_pending);
                     
  end else begin: gen_bc2 // block: gen_bc1

    // Next state transitions.
    always @(*)
    begin
      next_state = state;
      case (state)
        SM_IDLE:
          if (axvalid & data_ready)
            next_state = SM_CMD_EN;
          else
            next_state = state;
      
        SM_CMD_EN:
          if (~cmd_full & next_pending)
            next_state = SM_CMD_ACCEPTED;
          else if (~cmd_full & ~next_pending & b_full)
            next_state = SM_DONE_WAIT;
          else if (~cmd_full & ~next_pending & ~b_full)
            next_state = SM_IDLE;
          else
            next_state = state;            

        SM_CMD_ACCEPTED:
          if (data_ready )
            next_state = SM_CMD_EN;
          else 
            next_state = state;

        SM_DONE_WAIT:
          if (!b_full)
            next_state = SM_IDLE;
          else 
            next_state = state;

          default:
            next_state = SM_IDLE;
      endcase
    end

    // Assign outputs based on current state.

    assign cmd_en  =  (state == SM_CMD_EN);
                 
    always @(posedge clk) begin
      if (reset || (~init_complete_r1)) begin
        next_r <= 1'b0;
      end else begin
        next_r <= (((next_state == SM_CMD_ACCEPTED)  && (state != SM_CMD_ACCEPTED))
                  || ((next_state == SM_IDLE) && ((state == SM_CMD_EN) || (state == SM_DONE_WAIT))));
      end
    end

    assign next = next_r;

    assign r_push  = next;

    assign a_push  = (state == SM_IDLE);
    assign axready = ((state == SM_CMD_EN) | (state == SM_DONE_WAIT)) & (next_state == SM_IDLE);
    assign b_push  = ((state == SM_CMD_EN) | (state == SM_DONE_WAIT)) & (next_state == SM_IDLE);
    assign last_tran = (state == SM_CMD_EN) & (~next_pending);
                    
  end // block: gen_bc2
endgenerate
// push signal for w_cmd_rdy
// registered for timing 
assign w_push = (state == SM_IDLE) && axvalid && (~data_ready);

endmodule
`default_nettype wire
