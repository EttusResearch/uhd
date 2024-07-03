//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Module: license_check
//
// Description:
//
//   License checker for RFNoC blocks, using a combination of private key, device
//   serial, and feature flag. This module can be used to load a license key.
//   If the key is valid, it will assert an output signal, which can be consumed
//   by the module for which the license key is meant.
//
//   The license key is a combination of a feature identifier (32-bit) and a
//   SHA256 hash (256 bits). In total, the license key is thus 36 bytes long.
//
//   The hash is calculated from:
//   - A device serial
//   - A private key
//   - The 32-bit feature identifier
//
//   Usage:
//   - Upon instantiation, make sure that the serial input is connected to this
//     device's serial number.
//   - To unlock a feature, first write the feature ID to BASE_ADDR. This will
//     calculate a SHA256 hash.
//   - Then, write the rest of the user key in 32-bit words to BASE_ADDR+4. This
//     module will compare the user key with the internally generated hash bit
//     by bit.
//   - After all 8 32-bit words have been written, the feature enable line will
//     be asserted if the uploaded key matched the SHA256 hash.
//
//   Note that in this implementation, PKEY_W + SERIAL_W must be less than
//   512-32-64-8 as we require 32 bits for the feature flag, and 8 bits padding
//   (as well as the 64-bit message length).
//
// Registers (Relative to BASE_ADDR):
//   - FID_ADDR (BASE_ADDR): Feature ID (w)
//   - KEY_ADDR (BASE_ADDR+4): User key, one 32-bit word at a time (w)
//   - RB_ADDR (BASE_ADDR+8): Readback, will provide info about the last feature
//                            ID that was written. Bit 31 tells us if the feature
//                            was unlocked. The rest tells us which feature index
//                            it is.
//   - RB_FID_ADDR (BASE_ADDR+12): Readback, will return a feature flag this
//                                 module was compiled with. On the next read,
//                                 it will return the next feature ID. To
//                                 learn all feature IDs that this module knows,
//                                 keep reading this register until values
//                                 repeat.
//
// Parameters:
//
//   BASE_ADDR: Address for writing the feature flag. Key values are written to
//              BASE_ADDR+4 (see above).
//   PKEY_W: Width of private key (in bits).
//   SERIAL_W: Width of serial (in bits). See note above on available lengths.
//   NUM_FEATURES: Number of features that can be unlocked
//   FEATURE_IDS: A list of 32-bit values that identify features
//   PRIVATE_KEY: The private key
//   DEVICE_TYPE: For now, can only be ULTRASCALE.
//

`default_nettype none

module license_check #(
  parameter                         BASE_ADDR   = 0,
  parameter                         PKEY_W      = 312,
  parameter                         SERIAL_W    = 96,
  parameter                         NUM_FEATURES = 1,
  parameter [(32*NUM_FEATURES)-1:0] FEATURE_IDS = {32'hC0DE},
  parameter [PKEY_W-1:0]            PRIVATE_KEY = 0
)(
  input wire clk,
  input wire rst,

  input wire [SERIAL_W-1:0] serial,

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  output reg [NUM_FEATURES-1:0] feature_enabled = 0
);

  `include "../rfnoc/core/ctrlport.vh"


  //! Number of bits in the SHA256 message that encode the message length
  // (see SHA256 standard for details)
  localparam MSGLEN_W = 64;
  localparam FID_W    = 32; // Number of bits in feature ID
  // Number of bits that are being SHA-hashed
  localparam [MSGLEN_W-1:0] MSGLEN   = PKEY_W + SERIAL_W + FID_W;
  // We have 512 bits in a SHA-input block. The remaining bits that are not part
  // of the message are the padding and the message length.
  localparam PADDING_W = 512 - MSGLEN - MSGLEN_W; // Number of padding bits
  // We pad at least with a full byte. This wastes 7 bits of potential private
  // key length, but it also means we can't accidentally load invalid private
  // keys.
  if (PADDING_W < 8) begin : gen_assertion
    ERROR_max_key_len_exceeded();
  end
  // Padding bits (10000...)
  localparam [PADDING_W-1:0] PADDING = {1'b1, {(PADDING_W-1){1'b0}}};

  // Length of the feature indexer reg
  localparam FIDXREG_W  = $clog2(NUM_FEATURES)+1;
  // A value that is not a feature index
  localparam NO_FID = {FIDXREG_W{1'b1}};

  // Valid addresses on the CtrlPort bus
  localparam FID_ADDR = BASE_ADDR;
  localparam KEY_ADDR = BASE_ADDR + 4;
  localparam RB_ADDR  = BASE_ADDR + 8;
  localparam RB_FID_ADDR = BASE_ADDR + 12;

  // States
  typedef enum logic [2:0] {
    ST_IDLE,
    ST_CALC,
    ST_USERKEYDONE,
    ST_ACK,
    ST_CTRLPORT_ERROR
  } state_t;

  typedef enum logic {
    ST_RB_ENABLED,
    ST_RB_FIDS
  } rb_state_t;

  // Registers & Wires
  state_t state = ST_IDLE;
  reg       key_valid  = 1'b0; // Tracks if current hash matches
  reg       fid_hashed = 1'b0; // Tracks if we have hashed a feature ID
  reg [2:0] word_cnt   = 3'd7; // Track which word we are currently comparing
  // The index of the feature ID we're hashing/comparing
  reg [FIDXREG_W-1:0] feature_idx = NO_FID;
  // The index of the last feature ID we returned on RB_FID_ADDR
  reg [FIDXREG_W-1:0] rb_feature_idx = (NUM_FEATURES - 1);
  reg                 rb_last_req = ST_RB_ENABLED;

  wire         fid_wr_req = (s_ctrlport_req_wr && (s_ctrlport_req_addr == FID_ADDR));
  wire         key_wr_req = (s_ctrlport_req_wr && (s_ctrlport_req_addr == KEY_ADDR));
  wire [255:0] sha_digest;
  wire         sha_digest_valid;
  wire         sha_tready;
  wire         sha_tvalid  = fid_wr_req && (state == ST_IDLE);

  integer feat_i;

  wire [511:0] sha_input = {
                      serial,
                      PRIVATE_KEY,
                      s_ctrlport_req_data,
                      PADDING,
                      MSGLEN
                     };

  // Actual SHA256 calculation module
  sha256_stream sha256_inst (
    .clk            (clk),
    .rst            (rst),
    .mode           (1'b1), // Always SHA256, not SHA224
    .s_tdata_i      (sha_input),
    .s_tlast_i      (1'b1), // All SHA256 transactions in this module are single-block
    .s_tvalid_i     (sha_tvalid),
    .s_tready_o     (sha_tready),
    .digest_o       (sha_digest),
    .digest_valid_o (sha_digest_valid)
  );


  // State machine
  always @(posedge clk) begin
    if (rst) begin
      state      <= ST_IDLE;
      word_cnt   <= 3'd7;
      key_valid  <= 1'b0;
      fid_hashed <= 1'b0;
      // On reset, all features are disabled. Otherwise, they always stay on.
      feature_enabled <= {NUM_FEATURES{1'b0}};
      feature_idx     <= NO_FID;
      // Reset the readback feature index
      rb_feature_idx  <= (NUM_FEATURES - 1);
    end else case (state)
      ST_IDLE : begin
        //// Idle state: We're waiting on input via CtrlPort.
        // - If we get a feature flag to hash, we store it and start hashin'
        //   Note that the sha256 module is connected straight to the input and
        //   will start calculating when this condition is met.
        if (fid_wr_req) begin
          // If we switched to ST_CALC before sha_tready is asserted, then we
          // would read an invalid hash from the sha256 module. However, we wait
          // with returning an ACK when the sha256 module is busy, so we can
          // assume the hashing module is always ready in this scenario.
          // synthesis translate_off
          assert(sha_tready);
          // synthesis translate_on
          state      <= ST_CALC;
          word_cnt   <= 3'd7;
          key_valid  <= 1'b0;
          fid_hashed <= 1'b0;
          // Reset the feature index to a non-feature-index value
          feature_idx <= NO_FID;

          // We store which feature ID we've been comparing
          for (feat_i = 0; feat_i < NUM_FEATURES; feat_i = feat_i + 1) begin
            if (s_ctrlport_req_data == FEATURE_IDS[(32*feat_i) +: 32]) begin
              feature_idx <= feat_i;
            end
          end

        // - If we get a key to compare, but we didn't previously hash a feature
        //   ID, then we return an error code, because we can't compare it with
        //   anything yet!
        end else if (key_wr_req && !fid_hashed) begin
          state <= ST_CTRLPORT_ERROR;

        // - If we get a key to compare, and we *did* previously hash a flag, we
        //   start comparing word-for-word. At any point, if the comparison fails,
        //   our key becomes and stays invalid.
        end else if (key_wr_req) begin
          key_valid <= (word_cnt ==  7 ? (sha_digest[255:224] == s_ctrlport_req_data) :
                        word_cnt ==  6 ? (sha_digest[223:192] == s_ctrlport_req_data) :
                        word_cnt ==  5 ? (sha_digest[191:160] == s_ctrlport_req_data) :
                        word_cnt ==  4 ? (sha_digest[159:128] == s_ctrlport_req_data) :
                        word_cnt ==  3 ? (sha_digest[127: 96] == s_ctrlport_req_data) :
                        word_cnt ==  2 ? (sha_digest[ 95: 64] == s_ctrlport_req_data) :
                        word_cnt ==  1 ? (sha_digest[ 63: 32] == s_ctrlport_req_data) :
                                         (sha_digest[ 31:  0] == s_ctrlport_req_data))
                        && (word_cnt == 7 || key_valid);
          word_cnt <= word_cnt - 1;
          state    <= word_cnt == 0 ? ST_USERKEYDONE : ST_ACK;

        // Handle readback
        end else if (s_ctrlport_req_rd && (s_ctrlport_req_addr == RB_ADDR)) begin
          state <= ST_ACK;
          rb_last_req <= ST_RB_ENABLED;

        end else if (s_ctrlport_req_rd && (s_ctrlport_req_addr == RB_FID_ADDR)) begin
          state <= ST_ACK;
          rb_last_req <= ST_RB_FIDS;
          rb_feature_idx <= rb_feature_idx == (NUM_FEATURES-1)
              ? {FIDXREG_W{1'b0}}
              : (rb_feature_idx + 1);

        // Any other read/write request is ignored
        end else begin
          state <= ST_IDLE;
        end
      end // ST_IDLE
      ST_CALC : begin
        //// Hash calculation state:
        // We wait here until the SHA is calculated. We don't accept CtrlPort
        // transactions until then, either. Once the feature flag has been hashed,
        // we return to idle state and wait for a user key for comparison.
        fid_hashed <= sha_digest_valid;
        state      <= sha_digest_valid ? ST_IDLE : ST_CALC;
      end
      ST_USERKEYDONE : begin
        //// User-key-done state: We get here when 8 words have been provided for
        // comparison. If an invalid key is uploaded for a feature after it was
        // already unlocked, we leave the flag enabled.
        feature_enabled[feature_idx] <= key_valid | feature_enabled[feature_idx];
        state <= ST_ACK;
      end
      ST_ACK, ST_CTRLPORT_ERROR : state <= ST_IDLE;
      default : state <= ST_IDLE;
    endcase
  end // always @(posedge clk)

  assign s_ctrlport_resp_ack = (state == ST_CALC && sha_digest_valid) ||
                               (state == ST_ACK) ||
                               (state == ST_CTRLPORT_ERROR);
  assign s_ctrlport_resp_status =
                (state == ST_CTRLPORT_ERROR) ? CTRL_STS_CMDERR : CTRL_STS_OKAY;

  assign s_ctrlport_resp_data = (rb_last_req == ST_RB_ENABLED) ?
      ((feature_idx == NO_FID)
          ? 32'b0
          : {feature_enabled[feature_idx], {(31-FIDXREG_W){1'b0}}, feature_idx})
      : (FEATURE_IDS[(32*rb_feature_idx) +: 32]);

endmodule

`default_nettype wire
