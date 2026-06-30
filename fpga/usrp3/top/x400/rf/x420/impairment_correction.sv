// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: impairment_correction
//
// Description:
//
//  This module is used to correct IQ impairments in the received samples.
//  It adds a register interface to configure the IQ impairment correction blocks.
//
// Parameters:
//
//   NUM_SPC    : Number of samples per clock cycle
//   NUM_COEFFS : Number of coefficients for FIR filters
//

module impairment_correction #(
  parameter NUM_SPC     = 4,
  parameter NUM_COEFFS  = 15
) (
  input  logic clk,
  input  logic reset,

  // ----------- data path -----------
  // input signal
  input  logic [NUM_SPC*32-1:0] s_axis_tdata, // 32-bit samples (I (LSB) + Q (MSB))
  input  logic                  s_axis_tvalid,
  output logic                  s_axis_tready,

  // signal with IQ impairments applied
  output logic [NUM_SPC*32-1:0] m_axis_tdata, // 32-bit samples (I (LSB) + Q (MSB))
  output logic                  m_axis_tvalid,
  input  logic                  m_axis_tready,

  // configuration path
  ctrlport_if.slave s_ctrlport
);

  import XmlSvPkgIQ_IMPAIRMENT_REGMAP::*;
  import ctrlport_pkg::*;

  logic [24:0] s_axi_icross_coeff_tdata;
  logic s_axi_icross_coeff_tready;
  logic s_axi_icross_coeff_tvalid;

  logic [24:0] s_axi_qinline_coeff_tdata;
  logic s_axi_qinline_coeff_tready;
  logic s_axi_qinline_coeff_tvalid;

  logic [24:0] s_axi_iinline_coeff_tdata;
  logic s_axi_iinline_coeff_tready;
  logic s_axi_iinline_coeff_tvalid;

  logic [kGROUP_DELAYSize-1:0] group_delay;

  impairment_correction_dsp #(
    .NUM_SPC   (NUM_SPC),
    .NUM_COEFFS(NUM_COEFFS)
  ) impairment_dsp (
    .clk                       (clk),
    .reset                     (reset),
    .s_axis_tdata              (s_axis_tdata),
    .s_axis_tvalid             (s_axis_tvalid),
    .s_axis_tready             (s_axis_tready),
    .m_axis_tdata              (m_axis_tdata),
    .m_axis_tvalid             (m_axis_tvalid),
    .m_axis_tready             (m_axis_tready),
    .s_axi_icross_coeff_tdata  (s_axi_icross_coeff_tdata),
    .s_axi_icross_coeff_tvalid (s_axi_icross_coeff_tvalid),
    .s_axi_icross_coeff_tready (s_axi_icross_coeff_tready),
    .s_axi_qinline_coeff_tdata (s_axi_qinline_coeff_tdata),
    .s_axi_qinline_coeff_tvalid(s_axi_qinline_coeff_tvalid),
    .s_axi_qinline_coeff_tready(s_axi_qinline_coeff_tready),
    .s_axi_iinline_coeff_tdata (s_axi_iinline_coeff_tdata),
    .s_axi_iinline_coeff_tvalid(s_axi_iinline_coeff_tvalid),
    .s_axi_iinline_coeff_tready(s_axi_iinline_coeff_tready),
    .group_delay               (group_delay[$clog2(NUM_COEFFS)-1:0])
  );

  // control interface
  always_ff @(posedge clk) begin
    // default assignments
    s_ctrlport.resp.ack <= '0;
    s_ctrlport.resp.status <= STS_OKAY;
    s_ctrlport.resp.data <= 'X;

    // read access
    if (s_ctrlport.req.rd) begin
      // ack every read access
      s_ctrlport.resp.ack <= '1;
      // response based on address
      case (s_ctrlport.req.addr)
        kDSP_REG: begin
          automatic DSP_REG data_out = '0;
          data_out.NUM_COEFFS = NUM_COEFFS;
          s_ctrlport.resp.data <= data_out;
        end
        kDELAY_REG: begin
          automatic DELAY_REG data_out = '0;
          data_out.GROUP_DELAY = group_delay;
          s_ctrlport.resp.data <= data_out;
        end
        default: begin
          s_ctrlport.resp.status <= STS_CMDERR;
        end
      endcase

    // write access
    end else if (s_ctrlport.req.wr) begin
      // ack every write access
      s_ctrlport.resp.ack <= '1;
      // write to registers based on address
      case (s_ctrlport.req.addr)
        kDELAY_REG: begin
          automatic DELAY_REG data_in = s_ctrlport.req.data;
          group_delay <= data_in.GROUP_DELAY;
        end
        // the acknowledge is set in the AXI transfer below to wait for coefficients to be consumed
        kIINLINE_COEFF_REG: begin
          automatic IINLINE_COEFF_REG data_in = s_ctrlport.req.data;
          s_axi_iinline_coeff_tdata <= data_in.IINLINE_COEFF;
          s_axi_iinline_coeff_tvalid <= '1;
          s_ctrlport.resp.ack <= '0;
        end
        kICROSS_COEFF_REG: begin
          automatic ICROSS_COEFF_REG data_in = s_ctrlport.req.data;
          s_axi_icross_coeff_tdata <= data_in.ICROSS_COEFF;
          s_axi_icross_coeff_tvalid <= '1;
          s_ctrlport.resp.ack <= '0;
        end
        kQINLINE_COEFF_REG: begin
          automatic QINLINE_COEFF_REG data_in = s_ctrlport.req.data;
          s_axi_qinline_coeff_tdata <= data_in.QINLINE_COEFF;
          s_axi_qinline_coeff_tvalid <= '1;
          s_ctrlport.resp.ack <= '0;
        end
        default: begin
          s_ctrlport.resp.status <= STS_CMDERR;
        end
      endcase
    end

    // Handle AXI transfers of the coefficients
    // On data transfer data is reset and ack is set
    if (s_axi_iinline_coeff_tvalid && s_axi_iinline_coeff_tready) begin
      s_axi_iinline_coeff_tvalid <= '0;
      s_ctrlport.resp.ack <= '1;
    end
    if (s_axi_icross_coeff_tvalid && s_axi_icross_coeff_tready) begin
      s_axi_icross_coeff_tvalid <= '0;
      s_ctrlport.resp.ack <= '1;
    end
    if (s_axi_qinline_coeff_tvalid && s_axi_qinline_coeff_tready) begin
      s_axi_qinline_coeff_tvalid <= '0;
      s_ctrlport.resp.ack <= '1;
    end

    // reset only important control signals
    if (reset) begin
      s_axi_iinline_coeff_tvalid <= '0;
      s_axi_icross_coeff_tvalid  <= '0;
      s_axi_qinline_coeff_tvalid <= '0;
      s_ctrlport.resp.ack        <= '0;
    end
  end

/*XmlParse xml_on
<regmap name="IQ_IMPAIRMENT_REGMAP" readablestrobes="false">
  <group name="IQ_IMPAIRMENT_REGISTERS" size="0x020">
    <info>
      This register map contains the registers used to configure the IQ impairment correction block.

      The I output of the block is a delayed version of the I input scaled by the IINLINE_COEFF
      factor. The delay is determined by the GROUP_DELAY register and the processing delay of the
      FIR filters.

      The Q output of the block is a combination of the Q input and the I input. The I input is filtered
      by the I cross FIR filter and the Q input is filtered by the Q inline FIR filter.
      The outputs of both filters are added, rounded and clipped to calculate the Q output.

      The FIR filters boot up with all coefficients set to 0. Ensure to set a non-zero coefficient
      before running data through the filters.
    </info>

    <register name="DSP_REG" size="32" offset="0x00" attributes="Readable">
      <bitfield name="NUM_COEFFS" range="7..0" type="integer">
        <info>
          This field contains the number of coefficients used in the FIR filters.
        </info>
      </bitfield>
    </register>

    <register name="DELAY_REG" size="32" offset="0x04" attributes="Readable|Writable">
      <bitfield name="GROUP_DELAY" range="7..0" type="integer">
        <info>
          This value defines the group delay of the FIR filters based on the filter coefficients.
          It is used to delay the I path appropriately to match the Q path.
        </info>
      </bitfield>
    </register>

    <register name="IINLINE_COEFF_REG" size="32" offset="0x08" attributes="Writable">
      <bitfield name="IINLINE_COEFF" range="24..0" type="integer">
        <info>
          Writing to this register will change the coefficient on the I inline path.
          The coefficient is of type Q2.23 (ARM notation), which means the value of 1.0 would be
          represented as 0x800000.
          (see https://en.wikipedia.org/wiki/Q_(number_format)#ARM_version)
        </info>
      </bitfield>
    </register>

    <register name="ICROSS_COEFF_REG" size="32" offset="0x0C" attributes="Writable">
      <bitfield name="ICROSS_COEFF" range="24..0" type="integer">
        <info>
          Writing to this register will shift the coefficient into the I cross FIR filter block.
          Update the coefficients in the order NUM_COEFFS-1 to 0. NUM_COEFFS write
          accesses are needed to update all coefficients.
          The coefficients are of type Q2.23 (ARM notation), which means the value of 1.0 would be
          represented as 0x800000.
          (see https://en.wikipedia.org/wiki/Q_(number_format)#ARM_version)
        </info>
      </bitfield>
    </register>

    <register name="QINLINE_COEFF_REG" size="32" offset="0x10" attributes="Writable">
      <bitfield name="QINLINE_COEFF" range="24..0" type="integer">
        <info>
          Writing to this register will shift the coefficient into the Q inline FIR filter block.
          Update the coefficients in the order NUM_COEFFS-1 to 0. NUM_COEFFS write
          accesses are needed to update all coefficients.
          The coefficients are of type Q2.23 (ARM notation), which means the value of 1.0 would be
          represented as 0x800000.
          (see https://en.wikipedia.org/wiki/Q_(number_format)#ARM_version)
        </info>
      </bitfield>
    </register>

  </group>
</regmap>
XmlParse xml_off*/

endmodule
