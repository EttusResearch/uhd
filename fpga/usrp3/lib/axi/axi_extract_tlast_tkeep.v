//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_extract_tlast_tkeep
//
// Description:
//
//   This module extracts the TLAST and TKEEP values that were embedded by the
//   axi_embed_tlast_tkeep module. See axi_embed_tlast_tkeep for a description
//   of how the data is encoded.
//
//   Here are some extraction examples for DATA_W = 64.
//
//     0x1234567887654321 becomes
//     0x1234567887654321 (no changes)
//
//     0xDEADBEEF00000001 0x1234567887654321 becomes
//     0x1234567887654321 with TLAST=1 and TKEEP=0
//
//     0xDEADBEEF00000005 0x1234567887654321 becomes
//     0x1234567887654321 with TLAST=1 and TKEEP=2
//
//     0xDEADBEEF00000000 0xDEADBEEFFEEDCAFE
//     0xDEADBEEFFEEDCAFE without TLAST=0 and TKEEP=0 becomes
//
//     0xDEADBEEF00000002 0xDEADBEEFFEEDCAFE
//     0xDEADBEEFFEEDCAFE with TLAST=0 and TKEEP=1 becomes
//

module axi_extract_tlast_tkeep #(
   parameter DATA_W = 64,
   parameter KEEP_W = DATA_W  /8
) (
   input clk,
   input rst,

   // Input AXI-Stream
   input      [DATA_W-1:0] i_tdata,
   input                   i_tvalid,
   output reg              i_tready,

   // Output AXI-Stream
   output reg [DATA_W-1:0] o_tdata,
   output reg [KEEP_W-1:0] o_tkeep,
   output reg              o_tlast,
   output reg              o_tvalid,
   input                   o_tready
);

  localparam                  ESC_WORD_W = 32;
  localparam [ESC_WORD_W-1:0] ESC_WORD   = 'hDEADBEEF;


  //---------------------------------------------------------------------------
  // TKEEP and TLAST Holding Register
  //---------------------------------------------------------------------------

  reg              save_flags;
  reg              tlast_saved;
  reg [KEEP_W-1:0] tkeep_saved;

  always @(posedge clk) begin
    if (save_flags) begin
      // Save the TLAST and TKEEP values embedded in the escape word
      tlast_saved <= i_tdata[0];
      tkeep_saved <= i_tdata[1 +: KEEP_W];
    end
  end


  //--------------------------------------------------------------------------
  // State Machine
  //--------------------------------------------------------------------------

  localparam ST_IDLE = 0;
  localparam ST_DATA = 1;

  reg [0:0] state = ST_IDLE;
  reg [0:0] next_state;

  always @(posedge clk) begin
    if (rst) begin
      state <= ST_IDLE;
    end else begin
      state <= next_state;
    end
  end

  always @(*) begin
    // Default assignments (pass through)
    o_tdata    = i_tdata;
    o_tlast    = 1'b0;
    o_tkeep    = {KEEP_W{1'b1}};
    save_flags = 1'b0;
    next_state = state;
    o_tvalid   = i_tvalid;
    i_tready   = o_tready;

    case(state)
      //
      // Search for escape code. If found don't pass data downstream but
      // transition to next state. Otherwise, pass data downstream.
      //
      ST_IDLE: begin
        if ((i_tdata[DATA_W-1 -: ESC_WORD_W] == ESC_WORD) && i_tvalid) begin
          save_flags = 1'b1;
          next_state = ST_DATA;
          o_tvalid   = 1'b0;
          i_tready   = 1'b1;
        end
      end

      //
      // Output data word with the saved TLAST and TKEEP values
      //
      ST_DATA: begin
        o_tlast = tlast_saved;
        o_tkeep = tkeep_saved;
        if (i_tvalid & o_tready) begin
          next_state = ST_IDLE;
        end
      end
    endcase
  end

endmodule
