//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// DC offset correction block for AXI streams with more than 1 sample per cycle.
//
// Description:
//
//   Applies a configurable offset to a stream of IQ data.
//   The offset is subtracted from each sample in the stream.
//   Each sample is an AXI stream with 32-bit complex samples (16 bits I + 16 bits Q).
//   Configuration is done via ctrlport interface.
//
// Parameters:
//
//   NUM_SPC - Samples per cycle
//

module dc_offset #(
  parameter NUM_SPC     = 4
) (
  input  logic clk,
  input  logic reset,

  // ----------- data path -----------
  // input signal
  input  logic [NUM_SPC*32-1:0] s_axis_tdata, // 32-bit samples (I (LSB) + Q (MSB))
  input  logic                  s_axis_tvalid,
  output logic                  s_axis_tready,
  input  logic                  s_axis_tlast,

  // signal with DC offset applied
  output logic [NUM_SPC*32-1:0] m_axis_tdata, // 32-bit samples (I (LSB) + Q (MSB))
  output logic                  m_axis_tvalid,
  input  logic                  m_axis_tready,
  output logic                  m_axis_tlast,

  // configuration path
  ctrlport_if.slave s_ctrlport
);

import XmlSvPkgDC_OFFSET_REGMAP::*;
import ctrlport_pkg::*;

// Register storage
MODE_TYPE mode = BYPASS;
logic [15:0] i_offset = 'X;
logic [15:0] q_offset = 'X;

// Control interface
always_ff @(posedge clk) begin
  // Default assignments
  s_ctrlport.resp.ack    <= '0;
  s_ctrlport.resp.status <= STS_OKAY;
  s_ctrlport.resp.data   <= 'X;

  // Read access
  if (s_ctrlport.req.rd) begin
    s_ctrlport.resp.ack <= '1;
    case (s_ctrlport.req.addr)
      kCONTROL_REG: begin
        automatic CONTROL_REG data_out = '0;
        data_out.MODE = mode;
        s_ctrlport.resp.data <= data_out;
      end
      kOFFSET_VALUE_REG: begin
        automatic OFFSET_VALUE_REG data_out = '0;
        data_out.I_OFFSET_VALUE = i_offset;
        data_out.Q_OFFSET_VALUE = q_offset;
        s_ctrlport.resp.data <= data_out;
      end
      default: begin
        s_ctrlport.resp.status <= STS_CMDERR;
      end
    endcase

  // Write access
  end else if (s_ctrlport.req.wr) begin
    s_ctrlport.resp.ack <= '1;
    case (s_ctrlport.req.addr)
      kCONTROL_REG: begin
        automatic CONTROL_REG data_in = s_ctrlport.req.data;
        mode <= data_in.MODE;
      end
      kOFFSET_VALUE_REG: begin
        automatic OFFSET_VALUE_REG data_in = s_ctrlport.req.data;
        i_offset <= data_in.I_OFFSET_VALUE;
        q_offset <= data_in.Q_OFFSET_VALUE;
      end
      default: begin
        s_ctrlport.resp.status <= STS_CMDERR;
      end
    endcase
  end

  if (reset) begin
    mode <= BYPASS;
    s_ctrlport.resp.ack <= '0;
  end
end

// ---------------------------------------------------------------------------
// Data path
// ---------------------------------------------------------------------------
// Arrays for axi_round_and_clip_complex signals
logic [NUM_SPC-1:0] i_tready_arr;
logic [NUM_SPC-1:0] o_tvalid_arr;
logic [NUM_SPC-1:0] o_tlast_arr;

for (genvar spc = 0; spc < NUM_SPC; spc++) begin : gen_dc_offset
  logic signed [16:0] i_corr, q_corr; // 1 extra bit for sum

  always_comb begin
    logic signed [15:0] i_in, q_in;
    logic signed [15:0] i_offset_summand, q_offset_summand;

    // Unpack I and Q from input sample
    i_in = $signed(s_axis_tdata[spc*32 +: 16]);
    q_in = $signed(s_axis_tdata[spc*32+16 +: 16]);

    // Determine summand based on mode
    i_offset_summand = (mode == CORRECTION) ? $signed(i_offset) : 16'sd0;
    q_offset_summand = (mode == CORRECTION) ? $signed(q_offset) : 16'sd0;

    // Apply DC offset correction if mode is enabled
    i_corr = i_in - i_offset_summand;
    q_corr = q_in - q_offset_summand;
  end

  // Local signals to connect FIFO and round/clip
  logic [33:0] flop_tdata;
  logic        flop_tvalid, flop_tready, flop_tlast;

  // Insert flop2 FIFO for each stream
  axi_fifo_flop2 #(
    .WIDTH(35) // 17 bits I + 17 bits Q + tlast
  ) flop_inst (
    .clk(clk),
    .reset(reset),
    .clear(1'b0),
    .i_tdata({s_axis_tlast, q_corr, i_corr}),
    .i_tvalid(s_axis_tvalid),
    .i_tready(i_tready_arr[spc]),
    .o_tdata({flop_tlast, flop_tdata}),
    .o_tvalid(flop_tvalid),
    .o_tready(flop_tready),
    .space(),
    .occupied()
  );

  // Saturate if needed and pack back into 32-bit sample
  axi_round_and_clip_complex #(
    .WIDTH_IN(17),
    .WIDTH_OUT(16),
    .CLIP_BITS(1),
    .FIFOSIZE(1)
  ) round_and_clip_inst (
    .clk(clk),
    .reset(reset),
    .i_tdata(flop_tdata),
    .i_tlast(flop_tlast),
    .i_tvalid(flop_tvalid),
    .i_tready(flop_tready),
    .o_tdata(m_axis_tdata[spc*32 +: 32]),
    .o_tlast(o_tlast_arr[spc]),
    .o_tvalid(o_tvalid_arr[spc]),
    .o_tready(m_axis_tready)
  );
end

// Output assignments
assign s_axis_tready = &i_tready_arr; // Ready only when all are ready
assign m_axis_tvalid = |o_tvalid_arr; // Valid if any are valid
assign m_axis_tlast  = |o_tlast_arr;  // Last if any are last

endmodule


/*XmlParse xml_on
<regmap name="DC_OFFSET_REGMAP" readablestrobes="false">
  <group name="DC_OFFSET_REGISTERS" size="0x010">
    <info>
      This register map contains the registers used to configure the DC offset correction block.
    </info>

    <enumeratedtype name="MODE_TYPE">
      <value name="BYPASS" integer="0">
        <info>
          Bypass mode. The input samples are passed to the output without any processing.
        </info>
      </value>
      <value name="CORRECTION" integer="1">
        <info>
          Correction mode. The DC offset of @.OFFSET_VALUE_REG is applied to the input samples.
        </info>
      </value>
    </enumeratedtype>

    <register name="CONTROL_REG" size="32" offset="0x00" attributes="Readable|Writable">
      <bitfield name="MODE" range="0" type="MODE_TYPE" />
    </register>

    <register name="OFFSET_VALUE_REG" size="32" offset="0x04" attributes="Readable|Writable">
      <info>
        This register contains the DC offset value to be subtracted from the input samples.
        The value is a signed 16-bit integer in two's complement format.
        The LSB 16 bits are used for the I channel offset and the MSB 16 bits are used for the
        Q channel offset.
      </info>
      <bitfield name="I_OFFSET_VALUE" range="15..0" type="integer" />
      <bitfield name="Q_OFFSET_VALUE" range="31..16" type="integer" />
    </register>
  </group>
</regmap>
XmlParse xml_off*/
