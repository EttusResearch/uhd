// -- (c) Copyright 2011 - 2012 Xilinx, Inc. All rights reserved.
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
//-----------------------------------------------------------------------------
//
// Register Slice
//   Generic single-channel AXI pipeline register on forward and/or reverse signal path
//
// Verilog-standard:  Verilog 2001
//--------------------------------------------------------------------------
//
// Structure:
//   axic_sync_clock_converter
//     axic_sample_cycle_ratio
//
//--------------------------------------------------------------------------

`timescale 1ps/1ps
`default_nettype none

module ict106_axic_sync_clock_converter # (
///////////////////////////////////////////////////////////////////////////////
// Parameter Definitions
///////////////////////////////////////////////////////////////////////////////
  parameter C_FAMILY     = "virtex6",
  parameter integer C_PAYLOAD_WIDTH = 32,
  parameter integer C_S_ACLK_RATIO = 1,
  parameter integer C_M_ACLK_RATIO = 1 ,
  parameter integer C_MODE = 0  // 0 = light-weight (1-deep); 1 = fully-pipelined (2-deep)
  )
 (
///////////////////////////////////////////////////////////////////////////////
// Port Declarations
///////////////////////////////////////////////////////////////////////////////
  input wire                    SAMPLE_CYCLE_EARLY,
  input wire                    SAMPLE_CYCLE,
  // Slave side
  input  wire                        S_ACLK,
  input  wire                        S_ARESET,
  input  wire [C_PAYLOAD_WIDTH-1:0] S_PAYLOAD,
  input  wire                        S_VALID,
  output wire                        S_READY,

  // Master side
  input  wire                        M_ACLK,
  input  wire                        M_ARESET,
  output wire [C_PAYLOAD_WIDTH-1:0] M_PAYLOAD,
  output wire                        M_VALID,
  input  wire                        M_READY
);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Local parameters
////////////////////////////////////////////////////////////////////////////////
localparam [1:0] ZERO = 2'b10;
localparam [1:0] ONE  = 2'b11;
localparam [1:0] TWO  = 2'b01;
localparam [1:0] INIT = 2'b00;
localparam integer P_LIGHT_WT = 0;
localparam integer P_FULLY_REG = 1;

////////////////////////////////////////////////////////////////////////////////
// Wires/Reg declarations
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// BEGIN RTL
////////////////////////////////////////////////////////////////////////////////

generate
  if (C_S_ACLK_RATIO == C_M_ACLK_RATIO) begin : gen_passthru
    assign M_PAYLOAD = S_PAYLOAD;
    assign M_VALID   = S_VALID;
    assign S_READY   = M_READY;      
  end else begin : gen_sync_clock_converter
    wire s_sample_cycle;
    wire s_sample_cycle_early;
    wire m_sample_cycle;
    wire m_sample_cycle_early;

    wire slow_aclk;
    wire slow_areset;
    reg  s_areset_r;
    reg  m_areset_r;
   
    reg  s_tready_r; 
    wire s_tready_ns; 
    reg  m_tvalid_r; 
    wire m_tvalid_ns; 
    reg  [C_PAYLOAD_WIDTH-1:0] m_tpayload_r;
    reg  [C_PAYLOAD_WIDTH-1:0] m_tstorage_r;
    wire [C_PAYLOAD_WIDTH-1:0] m_tpayload_ns; 
    wire [C_PAYLOAD_WIDTH-1:0] m_tstorage_ns; 
    reg  m_tready_hold;
    wire m_tready_sample;
    wire load_tpayload;
    wire load_tstorage;
    wire load_tpayload_from_tstorage;
    reg [1:0] state;
    reg [1:0] next_state;
    
    always @(posedge S_ACLK) begin
      s_areset_r <= S_ARESET | M_ARESET;
    end
  
    always @(posedge M_ACLK) begin
      m_areset_r <= S_ARESET | M_ARESET;
    end

    assign slow_aclk   = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? M_ACLK   : S_ACLK;
    assign slow_areset = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? m_areset_r : s_areset_r;
    assign s_sample_cycle_early = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? SAMPLE_CYCLE_EARLY : 1'b1;
    assign s_sample_cycle       = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? SAMPLE_CYCLE : 1'b1;
    assign m_sample_cycle_early = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? 1'b1 : SAMPLE_CYCLE_EARLY;
    assign m_sample_cycle       = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? 1'b1 : SAMPLE_CYCLE;

    // Output flop for S_READY, value is encoded into state machine.
    assign s_tready_ns = (C_S_ACLK_RATIO > C_M_ACLK_RATIO) ? state[1] & (state != INIT) : next_state[1];

    always @(posedge S_ACLK) begin 
      if (s_areset_r) begin
        s_tready_r <= 1'b0;
      end
      else begin
        s_tready_r <= s_sample_cycle_early ? s_tready_ns : 1'b0;
      end
    end

    assign S_READY = s_tready_r;

    // Output flop for M_VALID
    assign m_tvalid_ns = next_state[0];

    always @(posedge M_ACLK) begin 
      if (m_areset_r) begin
        m_tvalid_r <= 1'b0;
      end
      else begin
        m_tvalid_r <= m_sample_cycle ? m_tvalid_ns : m_tvalid_r & ~M_READY;
      end
    end

    assign M_VALID = m_tvalid_r;

    // Hold register for M_READY when M_ACLK is fast.
    always @(posedge M_ACLK) begin 
      if (m_areset_r) begin
        m_tready_hold <= 1'b0;
      end
      else begin
        m_tready_hold <= m_sample_cycle ? 1'b0 : m_tready_sample;
      end
    end

    assign m_tready_sample = (M_READY ) | m_tready_hold;
    // Output/storage flops for PAYLOAD
    assign m_tpayload_ns = ~load_tpayload ? m_tpayload_r :
                           load_tpayload_from_tstorage ? m_tstorage_r : 
                           S_PAYLOAD;

    assign m_tstorage_ns = C_MODE ? (load_tstorage ? S_PAYLOAD : m_tstorage_r) : 0;

    always @(posedge slow_aclk) begin 
      m_tpayload_r <= m_tpayload_ns;
      m_tstorage_r <= C_MODE ? m_tstorage_ns : 0;
    end

    assign M_PAYLOAD = m_tpayload_r;

    // load logic
    assign load_tstorage = C_MODE && (state != TWO);
    assign load_tpayload = m_tready_sample || (state == ZERO);
    assign load_tpayload_from_tstorage = C_MODE && (state == TWO) && m_tready_sample;
    
    // State machine
    always @(posedge slow_aclk) begin 
      state <= next_state;
    end

    always @* begin 
      if (slow_areset) begin 
        next_state = INIT;
      end else begin
        case (state)
          INIT: begin
            next_state = ZERO;
          end
          // No transaction stored locally
          ZERO: begin
            if (S_VALID) begin
              next_state = C_MODE ? ONE : TWO; // Push from empty
            end
            else begin
              next_state = ZERO;
            end
          end

          // One transaction stored locally
          ONE: begin
            if (C_MODE == 0) begin
              next_state = TWO;  // State ONE is inaccessible when C_MODE=0
            end 
            else if (m_tready_sample & ~S_VALID) begin 
              next_state = ZERO; // Read out one so move to ZERO
            end
            else if (~m_tready_sample & S_VALID) begin
              next_state = TWO;  // Got another one so move to TWO
            end
            else begin
              next_state = ONE;
            end
          end

          // Storage registers full
          TWO: begin 
            if (m_tready_sample) begin
              next_state = C_MODE ? ONE : ZERO; // Pop from full
            end
            else begin
              next_state = TWO;
            end
          end
        endcase // case (state)
      end
    end
  end // gen_sync_clock_converter
  endgenerate
endmodule

`default_nettype wire
