//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

//! RFNoC specific digital up-conversion chain
// High level block diagram:
//
// HB1 -> HB2 -> CIC -> DDS/multiplier -> Scaler

// We don't care about framing here, hence no tlast

module duc #(
  parameter SR_PHASE_INC_ADDR = 0,
  parameter SR_SCALE_ADDR     = 1,
  parameter SR_INTERP_ADDR    = 2,
  parameter NUM_HB            = 2,
  parameter CIC_MAX_INTERP    = 128

)(
  input clk, input reset, input clear,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  input [31:0] i_tdata, input [127:0] i_tuser, input i_tvalid, output i_tready,
  output [31:0] o_tdata, output [127:0] o_tuser, output o_tvalid, input o_tready
);

  localparam RESET_DELAY = 3;

  localparam WIDTH  = 16; // Input/output bitwidth of the module
  localparam CWIDTH = 24; // Internal bitwidth needed for CORDIC accuracy
  localparam PWIDTH = 32; // Phase accumulator bitwidth

  reg  [1:0] hb_rate;             // Current Halfband rate
  reg  [7:0] cic_interp_rate;     // Current CIC rate

  wire [1:0] hb_rate_int;
  wire [7:0] cic_interp_rate_int;

  wire [2*CWIDTH-1:0] o_tdata_halfbands;  // Halfband output
  wire o_tvalid_halfbands;

  wire rate_changed;    // Rate changed by the settings registers
  wire reset_on_change; // Reset the halfbands and the cic everytime there is a rate change
  wire reset_on_live_change; // Reset when rate changes while streaming

  wire [PWIDTH-1:0] o_tdata_phase;
  wire o_tvalid_phase;
  wire o_tlast_phase;
  wire i_tready_phase;

  wire [17:0] scale_factor;

 /**************************************************************************
  * Settings registers
  **************************************************************************/
  // AXI settings bus for phase values
  axi_setting_reg #(
    .ADDR(SR_PHASE_INC_ADDR), .AWIDTH(8), .WIDTH(PWIDTH), .STROBE_LAST(1), .REPEATS(1))
  axi_sr_phase (
    .clk(clk), .reset(reset),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .o_tdata(o_tdata_phase), .o_tlast(o_tlast_phase), .o_tvalid(o_tvalid_phase), .o_tready(i_tready_phase));

  // AXI settings bus for scale
  setting_reg #(.my_addr(SR_SCALE_ADDR), .width(18)) sr_scale (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(scale_factor),.changed());

  // AXI settings bus for interpolation rate
  setting_reg #(.my_addr(SR_INTERP_ADDR), .width(10), .at_reset(1)) sr_interp
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out({hb_rate_int,cic_interp_rate_int}),.changed(rate_changed));

  // Changing interpolation rates while processing only when axi_rate_change sends a clear
  reg active, rate_changed_hold;
  reg [RESET_DELAY-1:0] shift_reset;
  always @(posedge clk) begin
    if (reset) begin
      active            <= 1'b0;
      rate_changed_hold <= 1'b0;
      cic_interp_rate   <= 'd1;
      hb_rate           <= 'd0;
      shift_reset       <= 'd0;
    end else begin
      if (clear | reset_on_change) begin
        active <= 1'b0;
      end else if (i_tready & i_tvalid) begin
        active <= 1'b1;
      end
      if (rate_changed & active) begin
        rate_changed_hold <= 1'b1;
      end
      if ((clear | ~active) & (rate_changed | rate_changed_hold)) begin
        rate_changed_hold <= 1'b0;
        cic_interp_rate   <= cic_interp_rate_int;
        hb_rate           <= hb_rate_int;
        shift_reset       <= {shift_reset[RESET_DELAY-1:0], 1'b1};
      end else begin
        shift_reset       <= {shift_reset[RESET_DELAY-1:0], 1'b0};
      end
    end
  end

  // Long reset for the halfbands
  assign reset_on_change = |shift_reset;
  assign reset_on_live_change =  (clear | reset_on_change | (~active & rate_changed));

 /**************************************************************************
  * Halfbands
  *************************************************************************/

  // Sign extend from 16 to 24 bits to increase the accuracy from the frequency shifter
  wire [2*CWIDTH-1:0] o_tdata_extd;

  sign_extend #(.bits_in(WIDTH), .bits_out(CWIDTH)) sign_extend_in_i (
    .in(i_tdata[2*WIDTH-1:WIDTH]), .out(o_tdata_extd[2*CWIDTH-1:CWIDTH]));

  sign_extend #(.bits_in(WIDTH), .bits_out(CWIDTH)) sign_extend_in_q (
    .in(i_tdata[WIDTH-1:0]), .out(o_tdata_extd[CWIDTH-1:0]));

  // Halfband 1 wires
  wire i_tready_hb1;
  wire [2*CWIDTH-1:0] o_tdata_hb1;
  wire o_tvalid_hb1, o_tready_hb1;
  // Halfband 2 wires
  wire i_tready_hb2;
  wire [2*CWIDTH-1:0] o_tdata_hb2;
  wire o_tvalid_hb2, o_tready_hb2;
  // Halfband 3 wires
  wire i_tready_hb3;
  wire [2*CWIDTH-1:0] o_tdata_hb3;
  wire o_tvalid_hb3, o_tready_hb3;
  generate
  if( NUM_HB > 0 ) begin
  axi_hb47 halfband1 (
    .aclk(clk),
    .aresetn(~(reset | clear | reset_on_change)),
    .s_axis_data_tvalid(i_tvalid),
    .s_axis_data_tready(i_tready_hb1),
    .s_axis_data_tdata(o_tdata_extd),
    .m_axis_data_tvalid(o_tvalid_hb1),
    .m_axis_data_tready(o_tready_hb1),
    .m_axis_data_tdata(o_tdata_hb1)
  );
  end else begin
  assign o_tdata_hb1 = 'h0;
  assign o_tvalid_hb1 = 1'h0;
  assign i_tready_hb1 = 1'b0;
  end
  if( NUM_HB > 1 ) begin
  axi_hb47 halfband2 (
    .aclk(clk),
    .aresetn(~(reset | clear | reset_on_change)),
    .s_axis_data_tvalid(o_tvalid_hb1),
    .s_axis_data_tready(i_tready_hb2),
    .s_axis_data_tdata({o_tdata_hb1[2*CWIDTH-1:CWIDTH] << 2, o_tdata_hb1[CWIDTH-1:0] << 2}),
    .m_axis_data_tvalid(o_tvalid_hb2),
    .m_axis_data_tready(o_tready_hb2),
    .m_axis_data_tdata(o_tdata_hb2)
  );
  end else begin
  assign o_tdata_hb2 = 'h0;
  assign o_tvalid_hb2 = 1'h0;
  assign i_tready_hb2 = 1'b0;
  end
  if( NUM_HB > 2 ) begin
  axi_hb47 halfband3 (
    .aclk(clk),
    .aresetn(~(reset | clear | reset_on_change)),
    .s_axis_data_tvalid(o_tvalid_hb2),
    .s_axis_data_tready(i_tready_hb3),
    .s_axis_data_tdata({o_tdata_hb2[2*CWIDTH-1:CWIDTH] << 2, o_tdata_hb2[CWIDTH-1:0] << 2}),
    .m_axis_data_tvalid(o_tvalid_hb3),
    .m_axis_data_tready(o_tready_hb3),
    .m_axis_data_tdata(o_tdata_hb3)
  );
  end else begin
  assign o_tdata_hb3 = 'h0;
  assign o_tvalid_hb3 = 1'h0;
  assign i_tready_hb3 = 1'b0;
  end
  endgenerate
 /**************************************************************************
  * Halfband selection multiplexing
  *************************************************************************/
  wire [2*CWIDTH-1:0] o_tdata_cic;
  wire [2*CWIDTH-1:0] o_cic;
  wire o_tvalid_cic, i_tready_cic;
  wire o_tready_cic;

  assign o_tdata_halfbands = (hb_rate == 2'b0) ? o_tdata_extd :
                             (hb_rate == 2'b1) ?  {o_tdata_hb1[2*CWIDTH-1:CWIDTH] << 2, o_tdata_hb1[CWIDTH-1:0] << 2} :
                             (hb_rate == 2'b10) ? {o_tdata_hb2[2*CWIDTH-1:CWIDTH] << 2, o_tdata_hb2[CWIDTH-1:0] << 2} :
                                                  {o_tdata_hb3[2*CWIDTH-1:CWIDTH] << 2, o_tdata_hb3[CWIDTH-1:0] << 2};
  // Clearing valid on rate change as the halfbands take 2 cycles to clear
  assign o_tvalid_halfbands = reset_on_live_change ? 1'b0 :
                              (hb_rate == 2'b0)    ?  i_tvalid :
                              (hb_rate == 2'b1)    ?  o_tvalid_hb1 :
                              (hb_rate == 2'b10)    ? o_tvalid_hb2 :
                                                      o_tvalid_hb3;
  // Throttle input data while rate change is going on
  assign i_tready     =  reset_on_live_change ? 1'b0 :
                         (hb_rate == 2'b0)    ? i_tready_cic :
                                                i_tready_hb1;
  assign o_tready_hb1 =  reset_on_live_change ? 1'b0 :
                         (hb_rate == 2'b1)    ? i_tready_cic :
                                                i_tready_hb2;
  assign o_tready_hb2 =  reset_on_live_change ? 1'b0 :
                         (hb_rate == 2'b10)   ? i_tready_cic :
                                                i_tready_hb3;

  assign o_tready_hb3 =  reset_on_live_change ? 1'b0 : i_tready_cic;

 /**************************************************************************
  * Ettus CIC; the Xilinx CIC has a minimum interpolation of 4,
  * so we use the strobed version and convert to and from AXI.
  *************************************************************************/
  wire to_cic_stb, from_cic_stb;
  wire [2*CWIDTH-1:0] to_cic_data;
  wire [CWIDTH-1:0] i_cic;
  wire [CWIDTH-1:0] q_cic;

  // Convert from AXI to strobed and back to AXI again for the CIC interpolation module
  axi_to_strobed #(.WIDTH(2*CWIDTH), .FIFO_SIZE(1), .MIN_RATE(128)) axi_to_strobed (
    .clk(clk), .reset(reset | reset_on_change), .clear(clear),
    .out_rate(cic_interp_rate), .ready(i_tready_cartesian & o_tready), .error(),
    .i_tdata(o_tdata_halfbands), .i_tvalid(o_tvalid_halfbands), .i_tlast(1'b0), .i_tready(i_tready_cic),
    .out_stb(to_cic_stb), .out_last(), .out_data(to_cic_data)
  );

  cic_interpolate #(.WIDTH(CWIDTH), .N(4), .MAX_RATE(CIC_MAX_INTERP)) cic_interpolate_i (
    .clk(clk), .reset(reset | clear | reset_on_change),
    .rate_stb(reset_on_change),
    .rate(cic_interp_rate), .strobe_in(to_cic_stb), .strobe_out(from_cic_stb),
    .signal_in(to_cic_data[2*CWIDTH-1:CWIDTH]), .signal_out(i_cic)
  );

  cic_interpolate #(.WIDTH(CWIDTH), .N(4), .MAX_RATE(CIC_MAX_INTERP)) cic_interpolate_q (
    .clk(clk), .reset(reset | clear | reset_on_change),
    .rate_stb(reset_on_change),
    .rate(cic_interp_rate), .strobe_in(to_cic_stb), .strobe_out(),
    .signal_in(to_cic_data[CWIDTH-1:0]), .signal_out(q_cic)
  );

  assign o_cic = {i_cic, q_cic};

  //FIFO_SIZE = 8 infers a bram fifo
  strobed_to_axi #(.WIDTH(2*CWIDTH), .FIFO_SIZE(8)) strobed_to_axi (
    .clk(clk), .reset(reset | reset_on_change), .clear(clear),
    .in_stb(from_cic_stb), .in_data(o_cic), .in_last(1'b0),
    .o_tdata(o_tdata_cic), .o_tvalid(o_tvalid_cic), .o_tlast(), .o_tready(o_tready_cic)
  );


 /**************************************************************************
  * Clip back to 16 bits
  *************************************************************************/
  wire o_tvalid_clip;

  axi_round_and_clip_complex #(
    .WIDTH_IN(CWIDTH), .WIDTH_OUT(WIDTH), .CLIP_BITS(CWIDTH-WIDTH)) // No rounding, all clip
  axi_round_and_clip_complex (
    .clk(clk), .reset(reset | clear | reset_on_change),
    .i_tdata(o_tdata_cic), .i_tlast(1'b0), .i_tvalid(o_tvalid_cic), .i_tready(o_tready_cic),
    .o_tdata(o_tdata), .o_tlast(), .o_tvalid(o_tvalid_clip), .o_tready(i_tready_cartesian));

  assign o_tvalid = reset_on_live_change ? 1'b0 : o_tvalid_clip;
  assign i_tready_cartesian = reset_on_live_change ? 1'b0 : o_tready;

  // Note: To facilitate timed tunes, the code has been moved outside
  //       the duc module to dds_timed.v.

endmodule // duc
