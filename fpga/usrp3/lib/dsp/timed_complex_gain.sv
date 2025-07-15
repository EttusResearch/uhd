//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: timed_complex_gain
//
// Description:
//
//  This module applies a complex gain to an input complex signal. The gain is applied
//  in a timed manner, allowing for precise control over when the gain is applied.
//
//  The gain can be updated via a control port interface, with the option to specify
//  a timestamp for when the new gain should take effect. If no timestamp is provided,
//  the gain will take effect immediately.
//
//  Caution: While this module is compatible with timed write commands sent from the
//  host, it violates the RFNoC specification by sending an acknowledge (ACK) for write
//  requests of the coefficient register when the transaction is received, and not
//  when the command is actually executed.
//
//  Timed read commands are not supported, and will result in a command error (CMDERR).
//
//  This module uses the RFNoC data item definition for complex data, where the upper bits
//  of the sample represent the real part and the lower bits represent the imaginary part.
//  See "RFNoC specification - Data Item and Component Ordering" for details.
//
// Parameters:
//   ITEM_W               : Width of the complex item
//   NIPC                 : Number of items per clock cycle
//   BASE_ADDR            : Base address for the control interface, used to
//                          specify which register space is used
//   TIMESTAMP_W          : Width of the timestamp
//   REG_ADDR_W           : Width of the address for the register interface
//   REG_DATA_W           : Width of the data for the register interface
//   COEFFICIENT_W        : Width of the gain coefficients
//   COEFF_FRAC_BITS      : Fractional bits for the gain coefficients
//   QUEUE_DEPTH          : Depth of the timed command queue
//   TS_SHIFT_REG_DEPTH   : Depth of the timestamp shift register to ease timing
//

`default_nettype none

module timed_complex_gain
import ctrlport_pkg::CTRLPORT_ADDR_W;
import ctrlport_pkg::CTRLPORT_DATA_W;
import rfnoc_chdr_utils_pkg::*;
#(
  parameter int ITEM_W             = 32,
  parameter int NIPC               = 2,
  parameter int BASE_ADDR          = 0,
  parameter int COEFFICIENT_W      = 16,
  parameter int COEFF_FRAC_BITS    = 14,
  parameter int QUEUE_DEPTH        = 5,
  parameter int TS_SHIFT_REG_DEPTH = 1
) (
  input wire clk,
  input wire rst,

  //----------------------------------------------------------------------------
  // Control Interface
  //----------------------------------------------------------------------------
  input  wire                         s_ctrlport_req_wr,
  input  wire                         s_ctrlport_req_rd,
  input  wire  [ CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  wire  [ CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  input  wire                         s_ctrlport_req_has_time,
  input  wire  [CHDR_TIMESTAMP_W-1:0] s_ctrlport_req_time,
  output logic                        s_ctrlport_resp_ack,
  output logic [                 1:0] s_ctrlport_resp_status,
  output logic [ CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  //----------------------------------------------------------------------------
  // Data Interface
  //----------------------------------------------------------------------------
  // Input and output strobe interfaces for complex data
  input  wire  [NIPC*ITEM_W-1:0]      data_in,       // Input complex data
  input  wire                         data_in_stb,   // Input data valid
  output logic [NIPC*ITEM_W-1:0]      data_out,      // Output complex data
  output logic                        data_out_stb,  // Output data valid
  input  wire  [CHDR_TIMESTAMP_W-1:0] timestamp      // Current timestamp for the data

);
  `include "timed_complex_gain_regs.svh"
  //----------------------------------------------------------------------------
  // Parameter checks
  //----------------------------------------------------------------------------
  if (ITEM_W % 2 != 0)
    $error("The item width must be even to accommodate real and imaginary parts.");
  if (COEFFICIENT_W > CTRLPORT_DATA_W / 2)
    $error("The coefficient width must be less than or equal to half of the",
           " ctrlport register data width to fit in a single request.");

  //----------------------------------------------------------------------------
  // Internal parameters
  //----------------------------------------------------------------------------
  localparam logic [COEFFICIENT_W-1:0]     COEFF_REAL_DEFAULT = 1'b1 << COEFF_FRAC_BITS;
  localparam logic [COEFFICIENT_W-1:0]     COEFF_IMAG_DEFAULT = '0;
  localparam logic [CHDR_TIMESTAMP_W-1:0]  MAX_TIMESTAMP      = {CHDR_TIMESTAMP_W{1'b1}};
  localparam logic [CHDR_TIMESTAMP_W-1:0]  DEFAULT_TIMESTAMP  = '0;

  localparam int NIPC_W = (NIPC > 1) ? $clog2(NIPC) : 1; // Bits to count NIPC items

  // Calculate the latency introduced by the timestamp shift register.
  // This is used to calibrate the timing of the coefficient updates
  // based on the current timestamp:
  //  - The latency is (TS_SHIFT_REG_DEPTH + GAIN_APPLY_LATENCY) clock cycles,
  //    as the timestamp is buffered through TS_SHIFT_REG_DEPTH registers, and
  //    the queued coefficient is applied after GAIN_APPLY_LATENCY clock cycles.
  //  - The latency is multiplied by NIPC, as NIPC samples are processed
  //    per clock cycle, and the timestamp is assumed to increment by
  //    NIPC per clock cycle.
  // This ensures that the samples received in the clock cycle corresponding
  // to the timestamp T0 will be the first samples to have the new coefficient
  // applied.
  localparam int GAIN_APPLY_LATENCY = 3;
  localparam int TIMESTAMP_LATENCY = (TS_SHIFT_REG_DEPTH + GAIN_APPLY_LATENCY) * NIPC;

  //----------------------------------------------------------------------------
  // Register Interface
  //----------------------------------------------------------------------------
  // Set default values for coefficient registers to  1.0 + 0.0j
  logic [    COEFFICIENT_W-1:0] reg_gain_real = COEFF_REAL_DEFAULT;
  logic [    COEFFICIENT_W-1:0] reg_gain_imag = COEFF_IMAG_DEFAULT;
  logic [(COEFFICIENT_W*2)-1:0] reg_queue_coeff_in;
  logic [ CHDR_TIMESTAMP_W-1:0] reg_timestamp_in;


  //----------------------------------------------------------------------------
  // Internal Interface Signals
  //----------------------------------------------------------------------------
  logic [           NIPC-1:0] _data_out_stb;

  //----------------------------------------------------------------------------
  // Queue Signals
  //----------------------------------------------------------------------------
  logic                         queue_valid_in;
  logic                         queue_ready_in;
  logic                         queue_valid_out;
  logic                         queue_ready_out;
  logic [                 15:0] queue_occupied;
  logic [    COEFFICIENT_W-1:0] real_coeff_queue_data_out;
  logic [    COEFFICIENT_W-1:0] imag_coeff_queue_data_out;
  logic [ CHDR_TIMESTAMP_W-1:0] timestamp_queue_data_out;

  //----------------------------------------------------------------------------
  // Timed command queue
  //----------------------------------------------------------------------------
  axi_fifo #(
    .WIDTH(2 * COEFFICIENT_W + CHDR_TIMESTAMP_W),
    .SIZE (QUEUE_DEPTH)
  ) coeff_ts_queue_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (),
    .i_tdata  ({reg_queue_coeff_in, reg_timestamp_in}),
    .i_tvalid (queue_valid_in),
    .i_tready (queue_ready_in),
    .o_tdata  ({real_coeff_queue_data_out,
              imag_coeff_queue_data_out,
              timestamp_queue_data_out}),
    .o_tvalid (queue_valid_out),
    .o_tready (queue_ready_out && data_in_stb),
    .space    (),
    .occupied(queue_occupied)
  );


  //----------------------------------------------------------------------------
  // Register Read/Write Interface
  //----------------------------------------------------------------------------
  always_ff @(posedge clk) begin : register_interface
    s_ctrlport_resp_ack    <= 1'b0;
    s_ctrlport_resp_data   <= '0;
    //--------------------------------------------------------------------------
    // Reads
    //--------------------------------------------------------------------------
    if (s_ctrlport_req_rd) begin
      case (s_ctrlport_req_addr)
        BASE_ADDR + REG_CGAIN_COEFF: begin
          s_ctrlport_resp_data <= {
            reg_gain_real,  // Real part of the gain
            reg_gain_imag   // Imaginary part of the gain
          };
          s_ctrlport_resp_ack <= 1'b1;  // Acknowledge the read request
        end
        default: begin
          // No-op; ack remains deasserted
        end
      endcase
    end

    //--------------------------------------------------------------------------
    // Writes
    //--------------------------------------------------------------------------
    if (s_ctrlport_req_wr) begin
      case (s_ctrlport_req_addr)
        BASE_ADDR + REG_CGAIN_COEFF: begin
          reg_queue_coeff_in <= s_ctrlport_req_data;
          if (s_ctrlport_req_has_time) begin
            reg_timestamp_in <= s_ctrlport_req_time;
          end else begin
            // Immediate execution if no time is provided
            reg_timestamp_in <= DEFAULT_TIMESTAMP;
          end
          queue_valid_in <= 1'b1;
        end
        default: begin
          // No-op; ack remains deasserted
        end
      endcase
    end

    // Send the acknowledge for write requests only if the queue is not full
    // This should not collide with other requests, as SW should wait until
    // the ack is received before sending another request.
    if (queue_valid_in && queue_ready_in) begin
      s_ctrlport_resp_ack <= 1'b1;  // Acknowledge the write request
      queue_valid_in      <= 1'b0;  // Clear the valid signal after enqueueing
    end

    if (rst) begin
      s_ctrlport_resp_ack  <= 1'b0; // Reset the acknowledge signal
      s_ctrlport_resp_data <= '0;   // Reset the response data
      queue_valid_in       <= 1'b0; // Clear the valid signal
    end
  end

  // Ctrlport status always responds with OKAY for valid read/write requests.
  assign s_ctrlport_resp_status = CTRL_STS_OKAY;  // OKAY

  //---------------------------------------------------------------------------
  // Timed Coefficient Updates
  //---------------------------------------------------------------------------
  // Buffer timebase to ease routing
  logic [CHDR_TIMESTAMP_W-1:0] timestamp_buffer[TS_SHIFT_REG_DEPTH];
  logic [CHDR_TIMESTAMP_W-1:0] last_popped_timestamp = MAX_TIMESTAMP;
  logic                        already_popped;
  logic                        update_last_popped;
  logic [NIPC_W-1:0]           sample_align;

  generate
    always_ff @(posedge clk) begin : timestamp_shift_reg
      if (data_in_stb) begin
        // Shift the timestamp through the register
        timestamp_buffer[0] <= timestamp + TIMESTAMP_LATENCY;
        for (int reg_depth = 1; reg_depth < TS_SHIFT_REG_DEPTH; reg_depth++) begin
          timestamp_buffer[reg_depth] <= timestamp_buffer[reg_depth-1];
        end
      end
    end
  endgenerate

  // Ensure only popping coefficients off the queue once per timestamp.
  // This assumes that there are no two subsequent coefficients queued with the
  // same timestamp.
  always_comb begin : check_last_popped_timestamp
      // Default values
      already_popped = (last_popped_timestamp == timestamp_queue_data_out);
      update_last_popped = timestamp_queue_data_out != DEFAULT_TIMESTAMP;
  end

  always_ff @(posedge clk) begin
    if (data_in_stb) begin
      queue_ready_out <= 1'b0;
    end
    // Calculate the sample alignment based on NIPC
    if (NIPC > 1) begin
      sample_align <= timestamp_queue_data_out[NIPC_W-1:0] -
        timestamp_buffer[TS_SHIFT_REG_DEPTH-1][NIPC_W-1:0];
    end else begin
      sample_align <= '0;
    end
    if (queue_valid_out && data_in_stb) begin
      // Check if timestamp in queue is current or has already passed.
      // Adjust timestamp by TS_SHIFT_REG_DEPTH, as it was buffered.
      if ( timestamp_queue_data_out<=
          timestamp_buffer[TS_SHIFT_REG_DEPTH-1]) begin
        // Check if the timestamp has already been applied
        if (!already_popped) begin
          // Update coefficients based on the timestamp
          reg_gain_real   <= real_coeff_queue_data_out;
          reg_gain_imag   <= imag_coeff_queue_data_out;
          if (update_last_popped) begin
            last_popped_timestamp <= timestamp_queue_data_out;
          end
          queue_ready_out <= 1'b1;
        end
      end
    end
    if (rst) begin
      reg_gain_real <= COEFF_REAL_DEFAULT;
      reg_gain_imag <= COEFF_IMAG_DEFAULT;
      queue_ready_out <= 1'b0;
      sample_align <= '0;
      last_popped_timestamp <= MAX_TIMESTAMP;
    end
  end

  //----------------------------------------------------------------------------
  // Complex Multiply Instances
  //----------------------------------------------------------------------------
  logic [(COEFFICIENT_W*2)-1:0] gain_coefficient[NIPC];

  // Assign the gain coefficients to each NIPC instance
  for (genvar item = 0; item < NIPC; item++) begin : gen_assign_coefficients
    always_ff @(posedge clk) begin
      if (queue_ready_out && data_in_stb) begin
        // Align the coefficient update based on NIPC:
        // Since we are checking one cycle ahead of time, sample_align of 0 means
        // that we are aligned to the timestamp and can therefore defer changing
        // the coefficient until the next clock cycle.
        // Since we are setting the reg_gain_* values in the same clock cycle
        // as we pop the queue, the assignment will automatically take effect
        // in the next clock cycle when queue_ready_out is deasserted.
        if (sample_align > '0) begin
          if (item >= sample_align) begin
            // Update the coefficient for this and subsequent items
            gain_coefficient[item] <= {reg_gain_real, reg_gain_imag};
          end
        end
      end else if (data_in_stb) begin
        // Hold the previous value if no new coefficient is available
        // This ensures that the coefficient is stable between updates
        gain_coefficient[item] <= {reg_gain_real, reg_gain_imag};
      end
    end
  end


  for (genvar item = 0; item < NIPC; item++) begin : gen_complex_multiply
    logic [ITEM_W-1:0] data_item;
    logic [ITEM_W-1:0] product;
    // Assign the data to the input
    assign data_item = data_in[item*ITEM_W+:ITEM_W];

    complex_multiply_stb #(
      .WIDTH_A                (COEFFICIENT_W),
      .WIDTH_B                (ITEM_W / 2),
      .WIDTH_PRODUCT          (ITEM_W / 2),
      .FRACTIONAL_BITS_A      (COEFF_FRAC_BITS),
      .FRACTIONAL_BITS_B      (0),
      .FRACTIONAL_BITS_PRODUCT(0)
    ) multiplication (
      .clk       (clk),
      .rst       (rst),
      .strobe_in (data_in_stb),
      .factor_a  (gain_coefficient[item]),
      .factor_b  (data_item),
      .product   (product),
      .strobe_out(_data_out_stb[item])
    );
    // Assign the product to the output
    assign data_out[item*ITEM_W+:ITEM_W] = product;
  end

  // Combine the output data and strobe signals
  assign data_out_stb = _data_out_stb[0];
endmodule

`default_nettype wire
