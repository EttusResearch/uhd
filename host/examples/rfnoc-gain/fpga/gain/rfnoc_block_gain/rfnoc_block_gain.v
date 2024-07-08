//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_gain
//
// Description:
//
//   This is an example RFNoC block. It applies a numeric gain to incoming
//   samples then outputs the result. A single register is used to control the
//   gain setting.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   IP_OPTION   : Select which IP to use for the complex multiply. Use one of
//                 the following options:
//                 HDL_IP         = In-tree RFNoC HDL, with a DSP48E1 primitive
//                 IN_TREE_IP     = In-tree "complex_multiplier" (Xilinx IP)
//                 OUT_OF_TREE_IP = Out-of-tree "cmplx_mul" (Xilinx IP)
//

`default_nettype none


module rfnoc_block_gain #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       IP_OPTION       = "HDL_IP"
) (
  // RFNoC Framework Clocks and Resets
  input  wire                   rfnoc_chdr_clk,
  input  wire                   rfnoc_ctrl_clk,
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [(1)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(1)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(1)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(1)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(1)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(1)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(1)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(1)-1:0]        m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready
);

  // These are examples of how to include an in-tree header file. UHD_FPGA_DIR
  // is defined automatically and can be referenced as needed. Tools vary
  // somewhat in how they support using macros in `include statements.
  //
  // This works in Vivado:
  //
  //   `include `"`UHD_FPGA_DIR/usrp3/lib/rfnoc/core/rfnoc_chdr_utils.vh`"
  //
  // Some tools allow this:
  //
  //   `define INCLUDE_UHD_FILE(REL_PATH) `"`UHD_FPGA_DIR/REL_PATH`"
  //   `include `INCLUDE_UHD_FILE(usrp3/lib/rfnoc/core/rfnoc_chdr_utils.vh)
  //
  // This should work in most tools:
  `define RFNOC_CHDR_UTILS_PATH `"`UHD_FPGA_DIR/usrp3/lib/rfnoc/core/rfnoc_chdr_utils.vh`"
  `include `RFNOC_CHDR_UTILS_PATH

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire               ctrlport_clk;
  wire               ctrlport_rst;
  wire               axis_data_clk;
  wire               axis_data_rst;
  // CtrlPort Master
  wire               m_ctrlport_req_wr;
  wire               m_ctrlport_req_rd;
  wire [19:0]        m_ctrlport_req_addr;
  wire [31:0]        m_ctrlport_req_data;
  reg                m_ctrlport_resp_ack;
  reg  [31:0]        m_ctrlport_resp_data;
  // Payload Stream to User Logic: in
  wire [32*1-1:0]    m_in_payload_tdata;
  wire [1-1:0]       m_in_payload_tkeep;
  wire               m_in_payload_tlast;
  wire               m_in_payload_tvalid;
  wire               m_in_payload_tready;
  // Context Stream to User Logic: in
  wire [CHDR_W-1:0]  m_in_context_tdata;
  wire [3:0]         m_in_context_tuser;
  wire               m_in_context_tlast;
  wire               m_in_context_tvalid;
  wire               m_in_context_tready;
  // Payload Stream from User Logic: out
  wire [32*1-1:0]    s_out_payload_tdata;
  wire [0:0]         s_out_payload_tkeep;
  wire               s_out_payload_tlast;
  wire               s_out_payload_tvalid;
  wire               s_out_payload_tready;
  // Context Stream from User Logic: out
  wire [CHDR_W-1:0]  s_out_context_tdata;
  wire [3:0]         s_out_context_tuser;
  wire               s_out_context_tlast;
  wire               s_out_context_tvalid;
  wire               s_out_context_tready;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_gain #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .MTU         (MTU)
  ) noc_shell_gain_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    // Reset Outputs
    .rfnoc_chdr_rst      (),
    .rfnoc_ctrl_rst      (),
    // RFNoC Backend Interface
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // CtrlPort Clock and Reset
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    // CtrlPort Master
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data      (m_ctrlport_resp_data),

    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk (axis_data_clk),
    .axis_data_rst (axis_data_rst),
    // Payload Stream to User Logic: in
    .m_in_payload_tdata  (m_in_payload_tdata),
    .m_in_payload_tkeep  (m_in_payload_tkeep),
    .m_in_payload_tlast  (m_in_payload_tlast),
    .m_in_payload_tvalid (m_in_payload_tvalid),
    .m_in_payload_tready (m_in_payload_tready),
    // Context Stream to User Logic: in
    .m_in_context_tdata  (m_in_context_tdata),
    .m_in_context_tuser  (m_in_context_tuser),
    .m_in_context_tlast  (m_in_context_tlast),
    .m_in_context_tvalid (m_in_context_tvalid),
    .m_in_context_tready (m_in_context_tready),
    // Payload Stream from User Logic: out
    .s_out_payload_tdata  (s_out_payload_tdata),
    .s_out_payload_tkeep  (s_out_payload_tkeep),
    .s_out_payload_tlast  (s_out_payload_tlast),
    .s_out_payload_tvalid (s_out_payload_tvalid),
    .s_out_payload_tready (s_out_payload_tready),
    // Context Stream from User Logic: out
    .s_out_context_tdata  (s_out_context_tdata),
    .s_out_context_tuser  (s_out_context_tuser),
    .s_out_context_tlast  (s_out_context_tlast),
    .s_out_context_tvalid (s_out_context_tvalid),
    .s_out_context_tready (s_out_context_tready)
  );

  //---------------------------------------------------------------------------
  // User Logic
  //---------------------------------------------------------------------------
  //
  // The code above this point is essentially unmodified from what was
  // generated by the tool. The code below implements the gain example.
  //
  // All registers are in the ctrlport_clk domain and the signal processing is
  // in the axis_data_clk domain. However, we specified in the block YAML
  // configuration file that we want both the control and data interfaces on
  // the rfnoc_chdr clock. So we don't need to worry about crossing the
  // register data from ctrlport_clk and axis_data_clk.
  //
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------
  //
  // There's only one register now, but we'll structure the register code to
  // make it easier to add more registers later.
  //
  //---------------------------------------------------------------------------

  localparam REG_GAIN_ADDR    = 0;    // Address for gain value
  localparam REG_GAIN_DEFAULT = 1;    // Default gain value

  reg [15:0] reg_gain = REG_GAIN_DEFAULT;

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      reg_gain = REG_GAIN_DEFAULT;
    end else begin
      // Default assignment
      m_ctrlport_resp_ack <= 0;

      // Handle read requests
      if (m_ctrlport_req_rd) begin
        case (m_ctrlport_req_addr)
          REG_GAIN_ADDR: begin
            m_ctrlport_resp_ack  <= 1;
            m_ctrlport_resp_data <= { 16'b0, reg_gain };
          end
        endcase
      end

      // Handle write requests
      if (m_ctrlport_req_wr) begin
        case (m_ctrlport_req_addr)
          REG_GAIN_ADDR: begin
            m_ctrlport_resp_ack <= 1;
            reg_gain            <= m_ctrlport_req_data[15:0];
          end
        endcase
      end
    end
  end

  //---------------------------------------------------------------------------
  // Signal Processing
  //---------------------------------------------------------------------------
  //
  // Multiply each complex sample by a real-valued gain. The RFNoC signals
  // m_in_payload_* and m_out_payload_* expect the data with the real/I
  // component on the upper bits [31:16] and the imaginary/Q component on the
  // lower bits [15:0].
  //
  // We only input the real-valued gain (reg_gain) when we have payload data to
  // go in (m_in_payload_*). That way the current gain value always applies to
  // the current sample. This assumes that the tready of both inputs have
  // identical behavior.
  //
  //---------------------------------------------------------------------------

  // Multiply result. I/real in [63:32], Q/imaginary in [31:0] (sc32).
  wire [63:0] mult_tdata;
  wire        mult_tlast;
  wire        mult_tvalid;
  wire        mult_tready;

  generate
    // Use a generate statement to choose which IP to use for the multiply.
    // These all do the same thing and we only have multiple options to show
    // how you can use IP from different locations.

    if (IP_OPTION == "HDL_IP") begin : gen_rfnoc_ip
      // Use the RFNoC mult_rc Verilog module, which uses a DSP48E1 primitive
      mult_rc #(
        .WIDTH_REAL (16),
        .WIDTH_CPLX (16),
        .WIDTH_P    (32),
        .DROP_TOP_P (5),     // Must be 5 for a normal multiply in DSP48E1
        .LATENCY    (4)      // Turn on all pipeline registers in the DSP48E1
      ) mult_rc_i (
        .clk         (axis_data_clk),
        .reset       (axis_data_rst),
        .real_tdata  (reg_gain),
        .real_tlast  (m_in_payload_tlast),
        .real_tvalid (m_in_payload_tvalid),
        .real_tready (),
        .cplx_tdata  (m_in_payload_tdata),
        .cplx_tlast  (m_in_payload_tlast),
        .cplx_tvalid (m_in_payload_tvalid),
        .cplx_tready (m_in_payload_tready),
        .p_tdata     (mult_tdata),
        .p_tlast     (mult_tlast),
        .p_tvalid    (mult_tvalid),
        .p_tready    (mult_tready)
      );

    end else if (IP_OPTION == "IN_TREE_IP") begin : gen_in_tree_ip
      // Use the "cmul" module, which uses the in-tree "complex_multiplier" IP.
      // This is a Xilinx Complex Multiplier LogiCORE IP located in the UHD
      // repository in fpga/usrp3/lib/ip/.

      // The LSB of the output is clipped in this IP, so double the gain to
      // compensate. This limits the maximum gain in this version.
      wire [15:0] gain = 2*reg_gain;

      cmul cmul_i (
        .clk      (axis_data_clk),
        .reset    (axis_data_rst),
        .a_tdata  ({gain, 16'b0}),
        .a_tlast  (m_in_payload_tlast ),
        .a_tvalid (m_in_payload_tvalid),
        .a_tready (),
        .b_tdata  (m_in_payload_tdata ),
        .b_tlast  (m_in_payload_tlast ),
        .b_tvalid (m_in_payload_tvalid),
        .b_tready (m_in_payload_tready),
        .o_tdata  (mult_tdata),
        .o_tlast  (mult_tlast),
        .o_tvalid (mult_tvalid),
        .o_tready (mult_tready)
      );

    end else if (IP_OPTION == "OUT_OF_TREE_IP") begin : gen_oot_ip
      // Use the out-of-tree "cmplx_mul" IP, which is a Xilinx Complex
      // Multiplier LogiCORE IP located in the IP directory of this example.

      // This IP expects real/I in the lower bits and imaginary/Q in the upper
      // bits, so we swap I and Q on the input and output to match RFNoC.
      //
      // This IP has a 33-bit output, but because it's AXI-Stream, each
      // component is placed in a 5-byte word. Since our gain is real only,
      // we'll never need all 33-bits.
      wire [79:0] m_axis_dout_tdata;

      cmplx_mul cmplx_mul_i (
        .aclk               (axis_data_clk),
        .aresetn            (~axis_data_rst),
        .s_axis_a_tdata     ({16'd0, reg_gain}),            // Real gain
        .s_axis_a_tlast     (m_in_payload_tlast),
        .s_axis_a_tvalid    (m_in_payload_tvalid),
        .s_axis_a_tready    (),
        .s_axis_b_tdata     ({m_in_payload_tdata[15:0],     // Imaginary
                              m_in_payload_tdata[31:16]}),  // Real
        .s_axis_b_tlast     (m_in_payload_tlast),
        .s_axis_b_tvalid    (m_in_payload_tvalid),
        .s_axis_b_tready    (m_in_payload_tready),
        .m_axis_dout_tdata  (m_axis_dout_tdata),
        .m_axis_dout_tlast  (mult_tlast),
        .m_axis_dout_tvalid (mult_tvalid),
        .m_axis_dout_tready (mult_tready)
      );

      assign mult_tdata[63:32] = m_axis_dout_tdata[31: 0]; // Real
      assign mult_tdata[31: 0] = m_axis_dout_tdata[71:40]; // Imaginary

    end
  endgenerate

  // Clip the results
  axi_clip_complex #(
    .WIDTH_IN  (32),
    .WIDTH_OUT (16)
  ) axi_clip_complex_i (
    .clk      (axis_data_clk),
    .reset    (axis_data_rst),
    .i_tdata  (mult_tdata),
    .i_tlast  (mult_tlast),
    .i_tvalid (mult_tvalid),
    .i_tready (mult_tready),
    .o_tdata  (s_out_payload_tdata),
    .o_tlast  (s_out_payload_tlast),
    .o_tvalid (s_out_payload_tvalid),
    .o_tready (s_out_payload_tready)
  );

  // Only 1-sample per clock, so tkeep should always be asserted
  assign s_out_payload_tkeep = 1'b1;

  // We're not doing anything fancy with the context (the CHDR header info) so
  // we can simply pass the input context through unchanged.
  assign s_out_context_tdata  = m_in_context_tdata;
  assign s_out_context_tuser  = m_in_context_tuser;
  assign s_out_context_tlast  = m_in_context_tlast;
  assign s_out_context_tvalid = m_in_context_tvalid;
  assign m_in_context_tready  = s_out_context_tready;

endmodule // rfnoc_block_gain


`default_nettype wire
