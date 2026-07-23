//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_axis
//
// Description:
//
//   Converts a CtrlPort write interface into an AXI-Stream bus. The module
//   contains an internal FIFO and will not acknowledge the write until the
//   FIFO has emptied enough to guarantee that another write can be accepted.
//
// Parameters:
//
//   WIDTH          : Width of the data path in bits.
//   FIFO_SIZE_LOG2 : Log base 2 of the internal FIFO size.
//
// Signals:
//
//   clk           : Clock
//   rst           : Active-high synchronous reset
//   ctrl_data     : CtrlPort data word
//   ctrl_wr       : CtrlPort write request strobe (single-cycle pulse)
//   ctrl_ack      : CtrlPort acknowledgement strobe (single-cycle pulse)
//   m_axis_tdata  : AXI-Stream data output
//   m_axis_tvalid : AXI-Stream valid
//   m_axis_tready : AXI-Stream ready (from downstream)
//

`default_nettype none


module ctrlport_to_axis #(
  parameter int WIDTH          = 32,
  parameter int FIFO_SIZE_LOG2 = 1
) (
  input  wire             clk,
  input  wire             rst,

  // CtrlPort write interface
  input  wire [WIDTH-1:0] ctrl_data,
  input  wire             ctrl_wr,
  output logic            ctrl_ack,

  // AXI-Stream master
  output wire  [WIDTH-1:0] m_axis_tdata,
  output wire              m_axis_tvalid,
  input  wire              m_axis_tready
);

  //---------------------------------------------------------------------------
  // Declarations
  //---------------------------------------------------------------------------

  typedef enum logic { ST_IDLE, ST_FULL } state_t;
  state_t state;

  logic        fifo_i_tready;
  logic [15:0] fifo_space;


  //---------------------------------------------------------------------------
  // State Machine
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin
    if (rst) begin
      state    <= ST_IDLE;
      ctrl_ack <= 1'b0;
    end else begin
      // Default assignments
      ctrl_ack <= 1'b0;

      unique case (state)
        ST_IDLE: begin
          if (ctrl_wr) begin
            if (fifo_space > 1 && FIFO_SIZE_LOG2 > 0) begin
              // If there's 2 or more spaces in the FIFO, acknowledge
              // immediately. If FIFO_SIZE_LOG2 is 0 (depth of 1), then the
              // space can never be greater than 1.
              ctrl_ack <= 1'b1;
            end else begin
              // If there's only one space, this write fills the FIFO, and we
              // can't acknowledge until we have enough room to accept the next
              // write.
              state <= ST_FULL;
            end
          end
        end

        ST_FULL: begin
          // Acknowledge the write as soon as space frees up in the FIFO.
          if (fifo_space > 0) begin
            ctrl_ack <= 1'b1;
            state <= ST_IDLE;
          end
        end

        default: begin
          state <= ST_IDLE;
        end
      endcase
    end
  end


  //---------------------------------------------------------------------------
  // Output FIFO
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH (WIDTH),
    .SIZE  (FIFO_SIZE_LOG2)
  ) data_fifo_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  (ctrl_data),
    .i_tvalid (ctrl_wr),
    .i_tready (fifo_i_tready),
    .o_tdata  (m_axis_tdata),
    .o_tvalid (m_axis_tvalid),
    .o_tready (m_axis_tready),
    .space    (fifo_space),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // Assertions
  //---------------------------------------------------------------------------

  if (FIFO_SIZE_LOG2 < 0) begin : gen_fifo_size_check
    $error("FIFO_SIZE_LOG2 must be >= 0");
  end

  //synthesis translate_off
  always_ff @(posedge clk) begin
    if (ctrl_wr && !fifo_i_tready) begin
      $error("ctrlport_to_axis: Write strobe occurred when the FIFO was not ready!");
    end
  end
  //synthesis translate_on


endmodule : ctrlport_to_axis


`default_nettype wire
