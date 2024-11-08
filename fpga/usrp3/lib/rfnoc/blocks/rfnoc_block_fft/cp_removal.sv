//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cp_removal
//
// Description:
//
//   Removes the cyclic prefix from OFDM symbols. This module assumes that each
//   AXI-stream input packet is one symbol with a prefix and it will output one
//   packet per symbol with the prefix removed. The cyclic prefix length to be
//   removed is input on the cp_len AXI-Stream input port, and it must be
//   present at the start of each data packet until cp_len_tready is asserted.
//   There is a one-cycle bubble at the start of each symbol to register cyclic
//   prefix length.
//
// Parameters:
//
//   CP_LEN_W  : Width of the maximum cyclic prefix length. The maximum
//               supported CP length is 2**CP_LEN_W - 1.
//   DATA_W    : Data/sample AXI-Stream bus width
//

`default_nettype none


module cp_removal #(
  int CP_LEN_W  = 12,
  int DATA_W    = 32
) (
  input  wire                 clk,
  input  wire                 rst,

  // Cyclic prefix length input port
  input  wire [ CP_LEN_W-1:0] cp_len_tdata,
  input  wire                 cp_len_tvalid,
  output reg                  cp_len_tready,

  // Symbol data stream input
  input  wire [   DATA_W-1:0] i_tdata,
  input  wire                 i_tlast,
  input  wire                 i_tvalid,
  output wire                 i_tready,

  // Symbol data stream output (one symbol per packet)
  output wire [   DATA_W-1:0] o_tdata,
  output wire                 o_tlast,
  output wire                 o_tvalid,
  input  wire                 o_tready
);
  `include "usrp_utils.svh"

  logic [CP_LEN_W-1:0] cp_len_reg;
  logic [CP_LEN_W-1:0] count = 1;

  enum logic [1:0] { ST_IDLE, ST_PREFIX, ST_BODY } state;

  always @(posedge clk) begin
    cp_len_tready <= 1'b0;

    case (state)
      // Wait in idle state until we get a new packet and the cyclic prefix
      // length.
      ST_IDLE : begin
        count <= 1;
        cp_len_reg <= cp_len_tdata;
        if (i_tvalid && cp_len_tvalid) begin
          cp_len_tready <= 1'b1;
          if (cp_len_tdata > 0) begin
            state <= ST_PREFIX;
          end else begin
            state <= ST_BODY;
          end
        end
      end

      // Remove the prefix
      ST_PREFIX : begin
        if (i_tvalid && i_tready) begin
          count <= count + 1;
          if (count == cp_len_reg) begin
            count <= 1;
            state <= ST_BODY;
          end
        end
      end

      // Pass through the rest until the end of the packet.
      ST_BODY : begin
        count <= 1;
        if (i_tvalid && i_tready) begin
          if(i_tlast) begin
            state <= ST_IDLE;
          end
        end
      end
    endcase

    if (rst) begin
      state         <= ST_IDLE;
      count         <= 'X;
      cp_len_reg    <= 'X;
      cp_len_tready <= '0;
    end
  end

  assign o_tdata  = i_tdata;
  assign o_tlast  = i_tlast;
  assign o_tvalid = (state == ST_BODY  ) ? i_tvalid : 1'b0;
  assign i_tready = (state == ST_BODY  ) ? o_tready :
                    (state == ST_PREFIX) ? 1'b1     : 1'b0;

endmodule : cp_removal

`default_nettype wire
