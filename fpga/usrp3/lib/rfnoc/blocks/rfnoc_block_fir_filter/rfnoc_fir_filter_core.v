//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//
//   Core module for a single instance of an FIR filter, implementing the
//   registers and signal processing for a single I/Q filter. It assumes the
//   data stream is an IQ pair with I in the upper 32 bits and Q is the lower
//   32 bits.
//
// Parameters:
//
//   DATA_W                   : Width of the input/output data stream to
//                              process.
//
//   BASE_ADDR                : Control port base address to which this block
//                              responds.
// 
//   COEFF_WIDTH              : Coefficient width
//
//   NUM_COEFFS               : Number of coefficients / filter taps
//
//   COEFFS_VEC               : Vector of NUM_COEFFS values, each of width
//                              COEFF_WIDTH, to initialize the filter
//                              coefficients. Defaults to an impulse.
//
//   RELOADABLE_COEFFS        : Enable (1) or disable (0) reloading
//                              coefficients at runtime
//
//   SYMMETRIC_COEFFS         : Reduce multiplier usage by approximately half
//                              if coefficients are symmetric
//
//   SKIP_ZERO_COEFFS         : Reduce multiplier usage by assuming zero valued
//                              coefficients in DEFAULT_COEFFS are always zero.
//                              Useful for halfband filters.
//
//   USE_EMBEDDED_REGS_COEFFS : Reduce register usage by only using embedded
//                              registers in DSP slices. Updating taps while
//                              streaming will cause temporary output
//                              corruption!
//
//   Note: If using USE_EMBEDDED_REGS_COEFFS, coefficients must be written at 
//   least once since COEFFS_VEC is ignored!


module rfnoc_fir_filter_core #(
  parameter        DATA_W            = 32,
  parameter [19:0] BASE_ADDR         = 0,

  // FIR Filter Parameters
  parameter COEFF_WIDTH              = 16,
  parameter NUM_COEFFS               = 41,
  parameter [NUM_COEFFS*COEFF_WIDTH-1:0] COEFFS_VEC = // Make impulse by default
    { 
      {1'b0, {(COEFF_WIDTH-1){1'b1}} },        // Max positive value
      {(COEFF_WIDTH*(NUM_COEFFS-1)){1'b0}}     // Zero for remaining coefficients
    },
  parameter RELOADABLE_COEFFS        = 1,
  parameter SYMMETRIC_COEFFS         = 0,
  parameter SKIP_ZERO_COEFFS         = 0,
  parameter USE_EMBEDDED_REGS_COEFFS = 1
) (

  input  wire clk,
  input  wire rst,

  //---------------------------------------------------------------------------
  // AXIS CTRL Port
  //---------------------------------------------------------------------------

  // Master
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Data Interface
  //---------------------------------------------------------------------------

  // Input data stream
  input  wire [DATA_W-1:0] s_axis_tdata,
  input  wire              s_axis_tlast,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,

  // Output data stream
  output wire [DATA_W-1:0] m_axis_tdata,
  output wire              m_axis_tlast,
  output wire              m_axis_tvalid,
  input  wire              m_axis_tready
);

  reg  [COEFF_WIDTH-1:0] m_axis_reload_tdata;
  reg                    m_axis_reload_tvalid;
  reg                    m_axis_reload_tlast;
  wire                   m_axis_reload_tready;


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  `include "rfnoc_fir_filter_regs.vh"

  // Separate the address into the block and register portions. Ignore the byte
  // offset.
  wire [20:0] block_addr = s_ctrlport_req_addr[19:FIR_FILTER_ADDR_W];
  wire [19:0] reg_addr   = { s_ctrlport_req_addr[FIR_FILTER_ADDR_W:2], 2'b0 };

  always @(posedge clk) begin
    if (rst) begin
      s_ctrlport_resp_ack  <= 0;
      m_axis_reload_tvalid <= 0;
      s_ctrlport_resp_data <= {32{1'bX}};
      m_axis_reload_tdata  <= {DATA_W{1'bX}};
      m_axis_reload_tlast  <= 1'bX;
    end else if (block_addr == BASE_ADDR) begin
      // Default assignments
      s_ctrlport_resp_ack  <= 0;
      s_ctrlport_resp_data <= 0;

      // Handle write acknowledgments. Don't ack the register write until it
      // gets accepted by the FIR filter.
      if (m_axis_reload_tvalid && m_axis_reload_tready) begin
        m_axis_reload_tvalid <= 1'b0;
        s_ctrlport_resp_ack  <= 1'b1;
      end

      // Handle register writes
      if (s_ctrlport_req_wr) begin
        if (reg_addr == REG_FIR_LOAD_COEFF) begin
          m_axis_reload_tdata  <= s_ctrlport_req_data[COEFF_WIDTH-1:0];
          m_axis_reload_tvalid <= 1'b1;
          m_axis_reload_tlast  <= 1'b0;
        end else if (reg_addr == REG_FIR_LOAD_COEFF_LAST) begin
          m_axis_reload_tdata  <= s_ctrlport_req_data[COEFF_WIDTH-1:0];
          m_axis_reload_tvalid <= 1'b1;
          m_axis_reload_tlast  <= 1'b1;
        end
      end

      // Handle register reads
      if (s_ctrlport_req_rd) begin
        // Ignore the upper bits so the we respond to any port
        if (reg_addr == REG_FIR_NUM_COEFFS) begin
          s_ctrlport_resp_data <= NUM_COEFFS;
          s_ctrlport_resp_ack  <= 1;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // FIR Filter Instances
  //---------------------------------------------------------------------------

  localparam IN_WIDTH  = DATA_W/2;
  localparam OUT_WIDTH = DATA_W/2;

  // I
  axi_fir_filter #(
    .IN_WIDTH                 (IN_WIDTH),
    .COEFF_WIDTH              (COEFF_WIDTH),
    .OUT_WIDTH                (OUT_WIDTH),
    .NUM_COEFFS               (NUM_COEFFS),
    .COEFFS_VEC               (COEFFS_VEC),
    .RELOADABLE_COEFFS        (RELOADABLE_COEFFS),
    .BLANK_OUTPUT             (1),
    // Optional optimizations
    .SYMMETRIC_COEFFS         (SYMMETRIC_COEFFS),
    .SKIP_ZERO_COEFFS         (SKIP_ZERO_COEFFS),
    .USE_EMBEDDED_REGS_COEFFS (USE_EMBEDDED_REGS_COEFFS)
  ) inst_axi_fir_filter_i (
    .clk                  (clk),
    .reset                (rst),
    .clear                (1'b0),
    .s_axis_data_tdata    (s_axis_tdata[2*IN_WIDTH-1:IN_WIDTH]),
    .s_axis_data_tlast    (s_axis_tlast),
    .s_axis_data_tvalid   (s_axis_tvalid),
    .s_axis_data_tready   (s_axis_tready),
    .m_axis_data_tdata    (m_axis_tdata[2*OUT_WIDTH-1:OUT_WIDTH]),
    .m_axis_data_tlast    (m_axis_tlast),
    .m_axis_data_tvalid   (m_axis_tvalid),
    .m_axis_data_tready   (m_axis_tready),
    .s_axis_reload_tdata  (m_axis_reload_tdata),
    .s_axis_reload_tlast  (m_axis_reload_tlast),
    .s_axis_reload_tvalid (m_axis_reload_tvalid),
    .s_axis_reload_tready (m_axis_reload_tready)
  );

  // Q
  axi_fir_filter #(
    .IN_WIDTH                 (IN_WIDTH),
    .COEFF_WIDTH              (COEFF_WIDTH),
    .OUT_WIDTH                (OUT_WIDTH),
    .NUM_COEFFS               (NUM_COEFFS),
    .COEFFS_VEC               (COEFFS_VEC),
    .RELOADABLE_COEFFS        (RELOADABLE_COEFFS),
    .BLANK_OUTPUT             (1),
    // Optional optimizations
    .SYMMETRIC_COEFFS         (SYMMETRIC_COEFFS),
    .SKIP_ZERO_COEFFS         (SKIP_ZERO_COEFFS),
    .USE_EMBEDDED_REGS_COEFFS (USE_EMBEDDED_REGS_COEFFS)
  ) inst_axi_fir_filter_q (
    .clk                  (clk),
    .reset                (rst),
    .clear                (1'b0),
    .s_axis_data_tdata    (s_axis_tdata[IN_WIDTH-1:0]),
    .s_axis_data_tlast    (s_axis_tlast),
    .s_axis_data_tvalid   (s_axis_tvalid),
    .s_axis_data_tready   (),
    .m_axis_data_tdata    (m_axis_tdata[OUT_WIDTH-1:0]),
    .m_axis_data_tlast    (),
    .m_axis_data_tvalid   (),
    .m_axis_data_tready   (m_axis_tready),
    .s_axis_reload_tdata  (m_axis_reload_tdata),
    .s_axis_reload_tlast  (m_axis_reload_tlast),
    .s_axis_reload_tvalid (m_axis_reload_tvalid),
    .s_axis_reload_tready ()
  );

endmodule
