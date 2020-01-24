//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_embed_tlast_tkeep
//
// Description:
//
//   This module takes the TLAST and TKEEP values of an AXI-Stream interface
//   and embeds them into the data stream. This allows a data pipe to be used
//   that isn't wide enough for the TDATA, TLAST,and TKEEP to be passed through
//   in parallel. Since TLAST and TKEEP are only usually needed for one word
//   per packet, this also reduces the amount of memory required to store a
//   packet. Note that this module only supports TKEEP at the end of a packet
//   when TLAST is asserted. See also axi_extract_tlast_tkeep.
//
//   This embedding is accomplished by using an escape sequence using the word
//   0xDEADBEEF as the escape code. If TLAST and TKEEP are both 0 (the usual
//   case) then no escape sequence is used. Any word that has "DEADBEEF" in the
//   most significant position is considered an escape word. The least
//   significant bits of the escape word contain the TKEEP and TLAST bits. The
//   word following the escape word is the normal data word associated with
//   those TLAST and TKEEP values.
//
//   Here are some examples for the case where DATA_W = 64
//
//     0x1234567887654321 with TLAST=0 and TKEEP=0 becomes
//     0x1234567887654321
//
//     0x1234567887654321 with TLAST=1 and TKEEP=0 becomes
//     0xDEADBEEF00000001 0x1234567887654321
//
//     0x1234567887654321 with TLAST=1 and TKEEP=2 becomes
//     0xDEADBEEF00000005 0x1234567887654321
//
//     0x1234567887654321 with TLAST=0 and TKEEP=1 becomes
//     0x1234567887654321 (because TKEEP is ignored when TLAST=0)
//
//     0xDEADBEEFFEEDCAFE without TLAST=0 and TKEEP=0 becomes
//     0xDEADBEEF00000000 0xDEADBEEFFEEDCAFE
//
//     0xDEADBEEFFEEDCAFE with TLAST=0 and TKEEP=1 becomes
//     0xDEADBEEF00000002 0xDEADBEEFFEEDCAFE
//

module axi_embed_tlast_tkeep #(
  parameter DATA_W = 64,
  parameter KEEP_W = DATA_W/8
) (
  input clk,
  input rst,

  // Input AXI-Stream
  input  [DATA_W-1:0] i_tdata,
  input  [KEEP_W-1:0] i_tkeep,
  input               i_tlast,
  input               i_tvalid,
  output              i_tready,

  // Output AXI-Stream
  output reg [DATA_W-1:0] o_tdata,
  output                  o_tvalid,
  input                   o_tready
);

  localparam                  ESC_WORD_W = 32;
  localparam [ESC_WORD_W-1:0] ESC_WORD   = 'hDEADBEEF;


  //---------------------------------------------------------------------------
  // Parameter Checking
  //---------------------------------------------------------------------------

  if (DATA_W < ESC_WORD_W+KEEP_W+1) begin : gen_assertion
    // Cause an error if DATA_W is not large enough.
    DATA_W_is_not_large_enough_to_store_escape_code_TKEEP_and_TLAST();
  end


  //---------------------------------------------------------------------------
  // State Machine
  //---------------------------------------------------------------------------

  localparam PASS   = 0;
  localparam ESCAPE = 1;

  localparam ST_IDLE = 0;
  localparam ST_DATA = 1;

  reg [0:0] state = ST_IDLE;
  reg [0:0] next_state;

  reg [0:0] select;

  always @(posedge clk) begin
    if (rst) begin
      state <= ST_IDLE;
    end else begin if (o_tready)
      state <= next_state;
    end
  end

  always @(*) begin
    case(state)
      ST_IDLE: begin
        if (i_tlast && i_tvalid) begin
          next_state = ST_DATA;
          select     = ESCAPE;
        end else if ((i_tdata[DATA_W-1 -: ESC_WORD_W] == ESC_WORD) && i_tvalid) begin
          next_state = ST_DATA;
          select     = ESCAPE;
        end else begin
          next_state = ST_IDLE;
          select     = PASS;
        end
      end

      ST_DATA: begin
        select = PASS;
        if (i_tvalid) begin
          next_state = ST_IDLE;
        end else begin
          next_state = ST_DATA;
        end
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // Output Multiplexers
  //---------------------------------------------------------------------------

  always @(*) begin
    case(select)
      PASS   : begin
        o_tdata = i_tdata;
      end
      ESCAPE : begin
        o_tdata                         = {DATA_W{1'b0}};
        o_tdata[DATA_W-1 -: ESC_WORD_W] = ESC_WORD;
        o_tdata[       1 +:     KEEP_W] = i_tkeep;
        o_tdata[       0 +:          1] = i_tlast;
      end
    endcase
  end

  assign o_tvalid = (select == PASS) ? i_tvalid : 1'b1;
  assign i_tready = (select == PASS) ? o_tready : 1'b0;

endmodule
