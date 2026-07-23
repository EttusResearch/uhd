//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_to_ctrlport
//
// Description:
//
//   Converts an AXI-Stream bus into a readable CtrlPort interface. Read
//   requests get responded to as soon as data becomes available on the
//   AXI-Stream input.
//
// Parameters:
//
//   WIDTH          : Width of the data path in bits
//   FIFO_SIZE_LOG2 : Log base 2 of the depth of the input AXI-Stream FIFO.
//                    This FIFO is optional. Depth can be set to -1 to remove
//                    it entirely.
//
// Signals:
//
//   clk           : Clock
//   rst           : Active-high synchronous reset
//   s_axis_tdata  : AXI-Stream data input
//   s_axis_tvalid : AXI-Stream valid
//   s_axis_tready : AXI-Stream ready
//   ctrl_rd       : CtrlPort read request strobe (single-cycle pulse)
//   ctrl_data     : CtrlPort read data output
//   ctrl_ack      : CtrlPort acknowledgement strobe (single-cycle pulse)
//

`default_nettype none


module axis_to_ctrlport #(
  parameter int WIDTH          = 32,
  parameter int FIFO_SIZE_LOG2 = 0
) (
  input  wire             clk,
  input  wire             rst,

  // AXI-Stream slave
  input  wire  [WIDTH-1:0] s_axis_tdata,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,

  // CtrlPort read interface
  input  wire              ctrl_rd,
  output logic [WIDTH-1:0] ctrl_data,
  output logic             ctrl_ack
);

  //---------------------------------------------------------------------------
  // Declarations
  //---------------------------------------------------------------------------

  typedef enum logic { ST_IDLE, ST_WAIT } state_t;
  state_t state;

  // Internal stream between the input FIFO and the FSM.
  logic [WIDTH-1:0] fifo_o_tdata;
  logic             fifo_o_tvalid;
  logic             fifo_o_tready;


  //---------------------------------------------------------------------------
  // Input FIFO
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH (WIDTH),
    .SIZE  (FIFO_SIZE_LOG2)
  ) input_fifo_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  (s_axis_tdata),
    .i_tvalid (s_axis_tvalid),
    .i_tready (s_axis_tready),
    .o_tdata  (fifo_o_tdata),
    .o_tvalid (fifo_o_tvalid),
    .o_tready (fifo_o_tready),
    .space    (),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // State Machine
  //---------------------------------------------------------------------------

  assign ctrl_data     = fifo_o_tdata;
  assign fifo_o_tready = ctrl_ack;

  always_ff @(posedge clk) begin
    if (rst) begin
      state         <= ST_IDLE;
      ctrl_ack      <= 1'b0;
    end else begin
      // Default: deassert ACK every cycle.
      ctrl_ack <= 1'b0;

      case (state)
        ST_IDLE: begin
          if (ctrl_rd) begin
            if (fifo_o_tvalid) begin
              // If we have data available, then ACK on next clock cycle.
              ctrl_ack <= 1'b1;
            end else begin
              // If we don't have data yet, then wait until it arrives.
              state <= ST_WAIT;
            end
          end
        end

        ST_WAIT: begin
          // ACK after we get a new data word
          if (fifo_o_tvalid) begin
            ctrl_ack <= 1'b1;
            state    <= ST_IDLE;
          end
        end

        default: begin
          state <= ST_IDLE;
        end
      endcase
    end
  end

endmodule : axis_to_ctrlport


`default_nettype wire
