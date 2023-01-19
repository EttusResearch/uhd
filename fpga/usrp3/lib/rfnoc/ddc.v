//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

//! RFNoC specific digital down-conversion chain

module ddc #(
  parameter SR_FREQ_ADDR     = 0,
  parameter SR_SCALE_IQ_ADDR = 1,
  parameter SR_DECIM_ADDR    = 2,
  parameter SR_MUX_ADDR      = 3,
  parameter SR_COEFFS_ADDR   = 4,
  parameter PRELOAD_HBS      = 1, // Preload half band filter state with 0s
  parameter NUM_HB           = 3,
  parameter CIC_MAX_DECIM    = 255,
  parameter SAMPLE_WIDTH     = 16,
  parameter WIDTH            = 24
)(
  input clk, input reset,
  input clear, // Resets everything except the timed phase inc FIFO and phase inc
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  input timed_set_stb, input [7:0] timed_set_addr, input [31:0] timed_set_data,
  input [31:0] sample_in_tdata,
  input sample_in_tvalid,
  input sample_in_tlast,
  output sample_in_tready,
  input sample_in_tuser,
  input sample_in_eob,
  output [31:0] sample_out_tdata,
  output sample_out_tvalid,
  input sample_out_tready,
  output sample_out_tlast
);

  localparam  cwidth = 25;
  localparam  zwidth = 24;

  wire [31:0] sr_phase_inc, sr_phase_inc_timed_tdata;
  wire sr_phase_inc_valid, sr_phase_inc_timed_tvalid, sr_phase_inc_timed_tready, sr_phase_inc_timed_tlast;
  reg [31:0] phase_inc;
  reg [31:0] phase;
  reg phase_inc_valid;
  
  wire [SAMPLE_WIDTH*2-1:0] dds_in_tdata;
  wire dds_in_tlast;
  wire dds_in_tvalid;
  wire dds_in_tready;
  wire [SAMPLE_WIDTH*2-1:0] dds_in_fifo_tdata;
  wire dds_in_fifo_tlast;
  wire dds_in_fifo_tvalid;
  wire dds_in_fifo_tready;
  wire [WIDTH-1:0] dds_in_i_tdata;
  wire [WIDTH-1:0] dds_in_q_tdata;  
  wire [WIDTH-1:0] dds_out_i_tdata;
  wire [WIDTH-1:0] dds_out_q_tdata;
  
  wire [SAMPLE_WIDTH*2-1:0] dds_in_sync_tdata;
  wire dds_in_sync_tvalid, dds_in_sync_tready, dds_in_sync_tlast;
  wire [WIDTH-1:0] phase_sync_tdata;
  wire phase_sync_tvalid, phase_sync_tready, phase_sync_tlast;  

  wire [WIDTH-1:0] phase_tdata = phase[31:32-WIDTH];
  wire phase_tvalid, phase_tready, phase_tlast;
  wire dds_out_tlast;
  wire dds_out_tvalid;
  wire [15:0] dds_input_fifo_space, dds_input_fifo_occupied;  

  wire [17:0] scale_factor;
  wire last_cic;
  wire last_cic_decimate_in;
  wire strobe_dds_clip;
  wire [WIDTH-1:0] i_dds_clip, q_dds_clip;
  wire [WIDTH-1:0] i_cic, q_cic;
  wire [46:0] i_hb1, q_hb1;
  wire [WIDTH-1:0] i_hb1_clip, q_hb1_clip;
  wire [46:0] i_hb2, q_hb2;
  wire [WIDTH-1:0] i_hb2_clip, q_hb2_clip;
  wire [47:0] i_hb3, q_hb3;
  wire [WIDTH-1:0] i_hb3_clip, q_hb3_clip;
  wire sample_out_stb;

  wire strobe_cic, strobe_hb1, strobe_hb2, strobe_hb3;
  wire ddc_chain_tready;

  reg [7:0] cic_decim_rate;
  wire [7:0] cic_decim_rate_int;
  wire rate_changed;

  wire [SAMPLE_WIDTH-1:0] sample_in_i = {sample_in_tdata[31:16]};
  wire [SAMPLE_WIDTH-1:0] sample_in_q = {sample_in_tdata[15:0]};

  wire sample_mux_tready;
  wire sample_mux_set_freq;
  wire [SAMPLE_WIDTH-1:0] sample_mux_i, sample_mux_q;
  wire realmode;
  wire swap_iq;

  reg [1:0] hb_rate;
  wire [1:0] hb_rate_int;
  wire [2:0] enable_hb = { hb_rate == 2'b11, hb_rate[1] == 1'b1, hb_rate != 2'b00 };

  wire reload_go, reload_we1, reload_we2, reload_we3, reload_ld1, reload_ld2, reload_ld3;
  wire [17:0] coef_din;

  //phase incr settings regs and mux.
  setting_reg #(.my_addr(SR_FREQ_ADDR)) set_freq (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(sr_phase_inc),.changed(sr_phase_inc_valid));

  assign sr_phase_inc_timed_tready = sample_in_tvalid & sample_in_tready & sample_mux_set_freq;

  axi_setting_reg #(
    .ADDR(SR_FREQ_ADDR),
    .USE_FIFO(1),
    .FIFO_SIZE(5))
  set_freq_timed (
    .clk(clk), .reset(reset), .error_stb(),
    .set_stb(timed_set_stb), .set_addr(timed_set_addr), .set_data(timed_set_data),
    .o_tdata(sr_phase_inc_timed_tdata), .o_tlast(sr_phase_inc_timed_tlast), .o_tvalid(sr_phase_inc_timed_tvalid),
    .o_tready(sr_phase_inc_timed_tready));

  // Load phase increment depending on whether or not the settings bus write is
  // a timed command. Non-timed commands get priority.
  always @(posedge clk) begin
    if (reset) begin
      phase_inc <= 'd0;
      phase_inc_valid <= 'd0;
    end else begin
      if (sr_phase_inc_valid) begin
        phase_inc <= sr_phase_inc;
        phase_inc_valid <= sr_phase_inc_valid;
      end else if (sr_phase_inc_timed_tvalid & sr_phase_inc_timed_tready) begin
        phase_inc <= sr_phase_inc_timed_tdata;
        phase_inc_valid <= sr_phase_inc_timed_tvalid;
      end else
        phase_inc_valid <= 1'b0;
    end
  end

  setting_reg #(.my_addr(SR_SCALE_IQ_ADDR), .width(18)) set_scale_iq (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(scale_factor),.changed());

  setting_reg #(.my_addr(SR_DECIM_ADDR), .width(10), .at_reset(1 /* No decimation */)) set_decim (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out({hb_rate_int, cic_decim_rate_int}),.changed(rate_changed));

  setting_reg #(.my_addr(SR_MUX_ADDR), .width(2)) set_mux (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out({realmode,swap_iq}),.changed());

  setting_reg #(.my_addr(SR_COEFFS_ADDR), .width(24)) set_coeffs (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out({reload_ld3,reload_we3,reload_ld2,reload_we2,reload_ld1,reload_we1,coef_din}),.changed(reload_go));

  // Prevent changing rate while processing samples as this
  // will corrupt the output
  reg active, rate_changed_hold, rate_changed_stb;
  always @(posedge clk) begin
    if (reset) begin
      active            <= 1'b0;
      rate_changed_hold <= 1'b0;
      rate_changed_stb  <= 1'b0;
      cic_decim_rate    <= 'd1;
      hb_rate           <= 'd0;
    end else begin
      if (clear) begin
        active <= 1'b0;
      end else if (sample_in_tvalid & sample_in_tready) begin
        active <= 1'b1;
      end
      if (rate_changed & active) begin
        rate_changed_hold <= 1'b1;
      end
      if ((clear | ~active) & (rate_changed | rate_changed_hold)) begin
        rate_changed_hold <= 1'b0;
        rate_changed_stb  <= 1'b1;
        cic_decim_rate    <= cic_decim_rate_int;
        hb_rate           <= hb_rate_int;
      end else begin
        rate_changed_stb  <= 1'b0;
      end
    end
  end


  //doesn't need to be registered and now can have back pressure from dds
  assign sample_mux_set_freq = sample_in_tuser;
  assign sample_mux_i = swap_iq ? sample_in_q : sample_in_i;
  assign sample_mux_q = realmode ? 'd0 : (swap_iq ? sample_in_i : sample_in_q);

  /** Phase accumulator, Xilinx DDS/Complex Mult **/
  
  //connect samples to dds
  assign dds_in_tdata = {sample_mux_i,sample_mux_q};
  assign dds_in_tvalid = sample_in_tvalid & ddc_chain_tready; //if the rest of the chain isn't ready, then halt all data flow. this should help with rate changes...
  assign dds_in_tlast = sample_in_tlast;
  assign sample_in_tready = dds_in_tready & ddc_chain_tready;
 
  assign phase_tvalid = dds_in_tvalid;
  assign phase_tlast = dds_in_tlast;
 
   // NCO
  always @(posedge clk) begin
    if (reset | clear | (phase_inc_valid & sr_phase_inc_timed_tready) | sample_in_eob) begin
      phase <= 0;
    end else if (dds_in_tvalid & dds_in_tready) begin //only increment phase when data is ready
      phase <= phase + phase_inc;
    end
  end

  // Sync the two path's pipeline delay.
  // This is needed to ensure that applying the phase update happens on the
  // correct sample regardless of differing downstream path delays.
  axi_sync #(
    .SIZE(2),
    .WIDTH_VEC({WIDTH,2*SAMPLE_WIDTH}), // Vector of widths, each width is defined by a 32-bit value
    .FIFO_SIZE(0))
  axi_sync (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({phase_tdata,dds_in_tdata}),
    .i_tlast({phase_tlast,dds_in_tlast}),
    .i_tvalid({phase_tvalid,dds_in_tvalid}),
    .i_tready({phase_tready,dds_in_tready}),
    .o_tdata({phase_sync_tdata,dds_in_sync_tdata}),
    .o_tlast({phase_sync_tlast,dds_in_sync_tlast}),
    .o_tvalid({phase_sync_tvalid,dds_in_sync_tvalid}),
    .o_tready({phase_sync_tready,dds_in_sync_tready}));

  //hold data to align with dds pipelining  
  axi_fifo #(.WIDTH(2*SAMPLE_WIDTH+1), .SIZE(5)) dds_input_fifo
    (.clk(clk), .reset(reset), .clear(clear),
    .i_tdata({dds_in_sync_tlast,dds_in_sync_tdata}), .i_tvalid(dds_in_sync_tvalid), .i_tready(dds_in_sync_tready),
    .o_tdata({dds_in_fifo_tlast,dds_in_fifo_tdata}), .o_tvalid(dds_in_fifo_tvalid), .o_tready(dds_in_fifo_tready),
    .space(dds_input_fifo_space), .occupied(dds_input_fifo_occupied)
    );
        
  // after fifo, do q quick sign extend op to get up to 24 bits. to match how the dds deals with the data path.
  // add extra bits to fit the dds width, 5 bits added here
  sign_extend #(
    .bits_in(SAMPLE_WIDTH), .bits_out(WIDTH))
  sign_extend_dds_i (
    .in({dds_in_fifo_tdata[2*SAMPLE_WIDTH-1:SAMPLE_WIDTH]}), .out(dds_in_i_tdata));

  sign_extend #(
    .bits_in(SAMPLE_WIDTH), .bits_out(WIDTH))
  sign_extend_dds_q (
    .in({dds_in_fifo_tdata[SAMPLE_WIDTH-1:0]}), .out(dds_in_q_tdata));      
  
  
  dds_freq_tune dds_freq_tune_inst (
    .clk(clk),
    .reset(reset | clear),
    .eob(sample_in_eob),
    .rate_changed(rate_changed_hold),
    .dds_input_fifo_occupied(dds_input_fifo_occupied),
    /* IQ input */
    .s_axis_din_tlast(dds_in_fifo_tlast),
    .s_axis_din_tvalid(dds_in_fifo_tvalid),
    .s_axis_din_tready(dds_in_fifo_tready),
    .s_axis_din_tdata({dds_in_q_tdata, dds_in_i_tdata}), //48 = WIDTH*2
    /* Phase input from NCO */
    .s_axis_phase_tvalid(phase_sync_tvalid),
    .s_axis_phase_tready(phase_sync_tready), // used in the axi_sync
    .s_axis_phase_tlast(phase_sync_tlast),
    .s_axis_phase_tdata(phase_sync_tdata), //24 bit = WIDTH
    /* IQ output */
    .m_axis_dout_tlast(dds_out_tlast),
    .m_axis_dout_tvalid(dds_out_tvalid),
    .m_axis_dout_tready(ddc_chain_tready), 
    .m_axis_dout_tdata({dds_out_q_tdata, dds_out_i_tdata})
        
  );

   //48 = WIDTH*2
  //chop off top byte because it's not actually used and we want to match expected gain/bit use found in freq shift
  assign i_dds_clip = {dds_out_i_tdata[15:0],8'h00};
  assign q_dds_clip = {dds_out_q_tdata[15:0],8'h00};
  assign strobe_dds_clip = dds_out_tvalid & sample_out_tready;
  assign last_cic_decimate_in = dds_out_tlast;
  
  /** CIC DECIMATE **/
  cic_decimate #(.WIDTH(WIDTH), .N(4), .MAX_RATE(CIC_MAX_DECIM)) cic_decimate_i (
    .clk(clk), .reset(reset | clear),
    .rate_stb(rate_changed_stb), .rate(cic_decim_rate), .strobe_in(strobe_dds_clip), .strobe_out(strobe_cic),
    .last_in(last_cic_decimate_in), .last_out(last_cic), .signal_in(i_dds_clip), .signal_out(i_cic));

  cic_decimate #(.WIDTH(WIDTH), .N(4), .MAX_RATE(CIC_MAX_DECIM)) cic_decimate_q (
    .clk(clk), .reset(reset | clear),
    .rate_stb(rate_changed_stb), .rate(cic_decim_rate), .strobe_in(strobe_dds_clip), .strobe_out(),
    .last_in(1'b0), .last_out(), .signal_in(q_dds_clip), .signal_out(q_cic));

  // Halfbands
  wire nd1, nd2, nd3;
  wire rfd1, rfd2, rfd3;
  wire rdy1, rdy2, rdy3;
  wire data_valid1, data_valid2, data_valid3;

  localparam HB1_SCALE = 18;
  localparam HB2_SCALE = 18;
  localparam HB3_SCALE = 18;

  // Track last sample as it propagates through the half band filters
  // Note: Delays calibrated for specific pipeline delay in each hb filter
  reg [5:0] hb1_in_cnt, hb2_in_cnt, hb3_in_cnt;
  reg [4:0] hb1_out_cnt, hb2_out_cnt, hb3_out_cnt;
  reg [4:0] hb1_last_cnt, hb2_last_cnt, hb3_last_cnt;
  reg hb1_last_set, hb2_last_set, hb3_last_set;
  reg last_hb1, last_hb2, last_hb3;
  always @(posedge clk) begin
    if (reset | clear) begin
      hb1_in_cnt   <= 'd0;
      hb2_in_cnt   <= 'd0;
      hb3_in_cnt   <= 'd0;
      hb1_out_cnt  <= 'd0;
      hb2_out_cnt  <= 'd0;
      hb3_out_cnt  <= 'd0;
      hb1_last_cnt <= 'd0;
      hb2_last_cnt <= 'd0;
      hb3_last_cnt <= 'd0;
      hb1_last_set <= 1'b0;
      hb2_last_set <= 1'b0;
      hb3_last_set <= 1'b0;
      last_hb1     <= 1'b0;
      last_hb2     <= 1'b0;
      last_hb3     <= 1'b0;
    end else begin
      // HB1
      if (strobe_cic & rfd1) begin
        hb1_in_cnt     <= hb1_in_cnt + 1'b1;
        if (last_cic) begin
          hb1_last_set <= 1'b1;
          hb1_last_cnt <= hb1_in_cnt[5:1];
        end
      end
      if (strobe_hb1) begin
        hb1_out_cnt    <= hb1_out_cnt + 1'b1;
      end
      // Avoid subtracting 1 from hb1_last_cnt by initializing hb1_out_cnt = 1
      if (hb1_last_set & (hb1_out_cnt == hb1_last_cnt)) begin
        last_hb1       <= 1'b1;
        hb1_last_set   <= 1'b0;
        hb1_last_cnt   <= 'd0;
      end else if (last_hb1 & strobe_hb1 & rfd2) begin
        last_hb1       <= 1'b0;
      end
      // HB2
      if (strobe_hb1 & rfd2) begin
        hb2_in_cnt   <= hb2_in_cnt + 1'b1;
        if (last_hb1) begin
          hb2_last_set <= 1'b1;
          hb2_last_cnt <= hb2_in_cnt[5:1];
        end
      end
      if (strobe_hb2) begin
        hb2_out_cnt    <= hb2_out_cnt + 1'b1;
      end
      if (hb2_last_set & (hb2_out_cnt == hb2_last_cnt)) begin
        last_hb2       <= 1'b1;
        hb2_last_set   <= 1'b0;
        hb2_last_cnt   <= 'd0;
      end else if (last_hb2 & strobe_hb2 & rfd3) begin
        last_hb2       <= 1'b0;
      end
      // HB3
      if (strobe_hb2 & rfd3) begin
        hb3_in_cnt     <= hb3_in_cnt + 1'b1;
        if (last_hb2) begin
          hb3_last_set <= 1'b1;
          hb3_last_cnt <= hb3_in_cnt[5:1];
        end
      end
      if (strobe_hb3) begin
        hb3_out_cnt    <= hb3_out_cnt + 1'b1;
      end
      if (hb3_last_set & (hb3_out_cnt == hb3_last_cnt)) begin
        last_hb3       <= 1'b1;
        hb3_last_set   <= 1'b0;
        hb3_last_cnt   <= 'd0;
      end else if (last_hb3 & strobe_hb3) begin
        last_hb3       <= 1'b0;
      end
    end
  end

  // Each filter will accept N-1 samples before outputting
  // a sample. This logic "preloads" the pipeline with 0s
  // so the first sample in pushes out a sample.
  reg [5:0] hb1_cnt, hb2_cnt, hb3_cnt;
  reg hb1_en, hb2_en, hb3_en, hb1_rdy, hb2_rdy, hb3_rdy;
  generate
    if (PRELOAD_HBS) begin
      always @(posedge clk) begin
        if (reset | clear) begin
          hb1_cnt <= 0;
          hb2_cnt <= 0;
          hb3_cnt <= 0;
          hb1_en  <= 1'b1;
          hb2_en  <= 1'b1;
          hb3_en  <= 1'b1;
          hb1_rdy <= 1'b0;
          hb2_rdy <= 1'b0;
          hb3_rdy <= 1'b0;
        end else begin
          if (hb1_en & rfd1) begin
            if (hb1_cnt < 47) begin
              hb1_cnt <= hb1_cnt + 1;
            end else begin
              hb1_en  <= 1'b0;
            end
          end
          if (data_valid1) begin
            hb1_rdy   <= 1'b1;
          end
          if (hb2_en & rfd2) begin
            if (hb2_cnt < 47) begin
              hb2_cnt <= hb2_cnt + 1;
            end else begin
              hb2_en  <= 1'b0;
            end
          end
          if (data_valid2) begin
            hb2_rdy   <= 1'b1;
          end
          if (hb3_en & rfd3) begin
            if (hb3_cnt < 63) begin
              hb3_cnt <= hb3_cnt + 1;
            end else begin
              hb3_en  <= 1'b0;
            end
          end
          if (data_valid3) begin
            hb3_rdy   <= 1'b1;
          end
        end
      end
    end else begin
      always @(*) begin
        hb1_en  <= 1'b0;
        hb2_en  <= 1'b0;
        hb3_en  <= 1'b0;
        hb1_rdy <= 1'b1;
        hb2_rdy <= 1'b1;
        hb3_rdy <= 1'b1;
      end
    end
  endgenerate

  assign ddc_chain_tready = sample_out_tready & hb1_rdy & hb2_rdy & hb3_rdy;

  assign strobe_hb1 = data_valid1 & hb1_rdy;
  assign strobe_hb2 = data_valid2 & hb2_rdy;
  assign strobe_hb3 = data_valid3 & hb3_rdy;
  assign nd1 = strobe_cic | hb1_en;
  assign nd2 = strobe_hb1 | hb2_en;
  assign nd3 = strobe_hb2 | hb3_en;
  generate //no point in using a for loop generate because each hb is different.
  if( NUM_HB > 0) begin
    hbdec1 hbdec1 (
      .clk(clk), // input clk
      .sclr(reset | clear), // input sclr
      .ce(1'b1), // input ce
      .coef_ld(reload_go & reload_ld1), // input coef_ld
      .coef_we(reload_go & reload_we1), // input coef_we
      .coef_din(coef_din), // input [17 : 0] coef_din
      .rfd(rfd1), // output rfd
      .nd(nd1), // input nd
      .din_1(i_cic), // input [23 : 0] din_1
      .din_2(q_cic), // input [23 : 0] din_2
      .rdy(rdy1), // output rdy
      .data_valid(data_valid1), // output data_valid
      .dout_1(i_hb1), // output [46 : 0] dout_1
      .dout_2(q_hb1)); // output [46 : 0] dout_2

    clip #(.bits_in(47-HB1_SCALE), .bits_out(WIDTH)) clip_hb1_i (
      .in(i_hb1[46:HB1_SCALE]),
      .out(i_hb1_clip)
    );

    clip #(.bits_in(47-HB1_SCALE), .bits_out(WIDTH)) clip_hb1_q (
      .in(q_hb1[46:HB1_SCALE]),
      .out(q_hb1_clip)
    );

  end else begin //if (NUM_HB == 0)
    assign rdy1 = 1'b1;
    assign rfd1 = 1'b1;
    assign data_valid1 = 1'b1;
    assign i_hb1 = 'h0;
    assign q_hb1 = 'h0;
    assign i_hb1_clip = 'h0;
    assign q_hb1_clip = 'h0;
  end
  if( NUM_HB > 1) begin
    hbdec2 hbdec2 (
      .clk(clk), // input clk
      .sclr(reset | clear), // input sclr
      .ce(1'b1), // input ce
      .coef_ld(reload_go & reload_ld2), // input coef_ld
      .coef_we(reload_go & reload_we2), // input coef_we
      .coef_din(coef_din), // input [17 : 0] coef_din
      .rfd(rfd2), // output rfd
      .nd(nd2), // input nd
      .din_1(i_hb1_clip), // input [23 : 0] din_1
      .din_2(q_hb1_clip), // input [23 : 0] din_2
      .rdy(rdy2), // output rdy
      .data_valid(data_valid2), // output data_valid
      .dout_1(i_hb2), // output [46 : 0] dout_1
      .dout_2(q_hb2)); // output [46 : 0] dout_2

    clip #(.bits_in(47-HB2_SCALE), .bits_out(WIDTH)) clip_hb2_i (
      .in(i_hb2[46:HB2_SCALE]),
      .out(i_hb2_clip)
    );

    clip #(.bits_in(47-HB2_SCALE), .bits_out(WIDTH)) clip_hb2_q (
      .in(q_hb2[46:HB2_SCALE]),
      .out(q_hb2_clip)
    );

  end else begin //if (NUM_HB <= 1)
    assign rdy2 = 1'b1;
    assign rfd2 = 1'b1;
    assign data_valid2 = 1'b1;
    assign i_hb2 = 'h0;
    assign q_hb2 = 'h0;
    assign i_hb2_clip = 'h0;
    assign q_hb2_clip = 'h0;
  end
  if( NUM_HB > 2) begin
    hbdec3 hbdec3 (
      .clk(clk), // input clk
      .sclr(reset | clear), // input sclr
      .ce(1'b1), // input ce
      .coef_ld(reload_go & reload_ld3), // input coef_ld
      .coef_we(reload_go & reload_we3), // input coef_we
      .coef_din(coef_din), // input [17 : 0] coef_din
      .rfd(rfd3), // output rfd
      .nd(nd3), // input nd
      .din_1(i_hb2_clip), // input [23 : 0] din_1
      .din_2(q_hb2_clip), // input [23 : 0] din_2
      .rdy(rdy3), // output rdy
      .data_valid(data_valid3), // output data_valid
      .dout_1(i_hb3), // output [47 : 0] dout_1
      .dout_2(q_hb3)); // output [47 : 0] dout_2

    clip #(.bits_in(48-HB3_SCALE), .bits_out(WIDTH)) clip_hb3_i (
      .in(i_hb3[47:HB3_SCALE]),
      .out(i_hb3_clip)
    );

    clip #(.bits_in(48-HB3_SCALE), .bits_out(WIDTH)) clip_hb3_q (
      .in(q_hb3[47:HB3_SCALE]),
      .out(q_hb3_clip)
    );

  end else begin //if (NUM_HB <= 2)
    assign rdy3 = 1'b1;
    assign rfd3 = 1'b1;
    assign data_valid3 = 1'b1;
    assign i_hb3 = 'h0;
    assign q_hb3 = 'h0;
    assign i_hb3_clip = 'h0;
    assign q_hb3_clip = 'h0;
  end
  endgenerate
  reg [23:0] i_unscaled, q_unscaled;
  reg strobe_unscaled;
  reg last_unscaled;
  //this state machine must be changed if the user wants 4 hbs
  always @(posedge clk) begin
    if (reset | clear) begin
      i_unscaled <= 'd0;
      q_unscaled <= 'd0;
      last_unscaled <= 1'b0;
      strobe_unscaled <= 1'b0;
    end else begin
      case(hb_rate)
        2'd0 : begin
          last_unscaled <= last_cic;
          strobe_unscaled <= strobe_cic;
          i_unscaled <= i_cic[23:0];
          q_unscaled <= q_cic[23:0];
        end
        2'd1 : begin
          last_unscaled <= last_hb1;
          strobe_unscaled <= strobe_hb1;
          i_unscaled <= i_hb1_clip;
          q_unscaled <= q_hb1_clip;
        end
        2'd2 : begin
          last_unscaled <= last_hb2;
          strobe_unscaled <= strobe_hb2;
          i_unscaled <= i_hb2_clip;
          q_unscaled <= q_hb2_clip;
        end
        2'd3 : begin
          last_unscaled <= last_hb3;
          strobe_unscaled <= strobe_hb3;
          i_unscaled <= i_hb3_clip;
          q_unscaled <= q_hb3_clip;
        end
      endcase // case (hb_rate)
    end
  end

  wire [42:0] i_scaled, q_scaled;
  wire [23:0] i_clip, q_clip;
  reg         strobe_scaled;
  reg         last_scaled;
  wire        strobe_clip;
  reg [1:0]   last_clip;

  MULT_MACRO #(
    .DEVICE("7SERIES"),     // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
    .LATENCY(1),            // Desired clock cycle latency, 0-4
    .WIDTH_A(25),           // Multiplier A-input bus width, 1-25
    .WIDTH_B(18))           // Multiplier B-input bus width, 1-18
  SCALE_I (.P(i_scaled),    // Multiplier output bus, width determined by WIDTH_P parameter
    .A({i_unscaled[23],i_unscaled}),     // Multiplier input A bus, width determined by WIDTH_A parameter
    .B(scale_factor),                    // Multiplier input B bus, width determined by WIDTH_B parameter
    .CE(strobe_unscaled),   // 1-bit active high input clock enable
    .CLK(clk),              // 1-bit positive edge clock input
    .RST(reset | clear));   // 1-bit input active high reset

  MULT_MACRO #(
    .DEVICE("7SERIES"),     // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
    .LATENCY(1),            // Desired clock cycle latency, 0-4
    .WIDTH_A(25),           // Multiplier A-input bus width, 1-25
    .WIDTH_B(18))           // Multiplier B-input bus width, 1-18
   SCALE_Q (.P(q_scaled),   // Multiplier output bus, width determined by WIDTH_P parameter
    .A({q_unscaled[23],q_unscaled}),     // Multiplier input A bus, width determined by WIDTH_A parameter
    .B(scale_factor),                    // Multiplier input B bus, width determined by WIDTH_B parameter
    .CE(strobe_unscaled),   // 1-bit active high input clock enable
    .CLK(clk),              // 1-bit positive edge clock input
    .RST(reset | clear));   // 1-bit input active high reset

  wire [31:0] sample_out;
  reg sample_out_last;

  always @(posedge clk) begin
    if (reset | clear) begin
      strobe_scaled   <= 1'b0;
      last_scaled     <= 1'b0;
      last_clip       <= 'd0;
      sample_out_last <= 1'b0;
    end else begin
      strobe_scaled   <= strobe_unscaled;
      last_scaled     <= last_unscaled;
      last_clip[1:0]  <= {last_clip[0], last_scaled};
      sample_out_last <= last_clip[1];
    end
  end

  clip_reg #(.bits_in(29), .bits_out(24), .STROBED(1)) clip_i (
    .clk(clk), .reset(reset | clear), .in(i_scaled[42:14]), .strobe_in(strobe_scaled), .out(i_clip), .strobe_out(strobe_clip));
  clip_reg #(.bits_in(29), .bits_out(24), .STROBED(1)) clip_q (
    .clk(clk), .reset(reset | clear), .in(q_scaled[42:14]), .strobe_in(strobe_scaled), .out(q_clip), .strobe_out());

  round_sd #(.WIDTH_IN(24), .WIDTH_OUT(16), .DISABLE_SD(1)) round_i (
    .clk(clk), .reset(reset | clear), .in(i_clip), .strobe_in(strobe_clip), .out(sample_out[31:16]), .strobe_out(sample_out_stb));
  round_sd #(.WIDTH_IN(24), .WIDTH_OUT(16), .DISABLE_SD(1)) round_q (
    .clk(clk), .reset(reset | clear), .in(q_clip), .strobe_in(strobe_clip), .out(sample_out[15:0]), .strobe_out());

  //FIFO_SIZE = 8 infers a bram fifo
  strobed_to_axi #(
    .WIDTH(32),
    .FIFO_SIZE(8))
  strobed_to_axi (
    .clk(clk), .reset(reset), .clear(clear),
    .in_stb(sample_out_stb), .in_data(sample_out), .in_last(sample_out_last),
    .o_tdata(sample_out_tdata), .o_tlast(sample_out_tlast), .o_tvalid(sample_out_tvalid), .o_tready(sample_out_tready));

endmodule // ddc_chain
