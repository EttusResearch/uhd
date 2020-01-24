//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


//! The USRP digital up-conversion chain

module duc_chain
  #(
    parameter BASE = 0,
    parameter DSPNO = 0,
    parameter WIDTH = 24,
    parameter NEW_HB_INTERP = 0,
    parameter DEVICE = "7SERIES"
  )
  (input clk, input rst, input clr,
   input set_stb, input [7:0] set_addr, input [31:0] set_data,

   // To TX frontend
   output [WIDTH-1:0] tx_fe_i,
   output [WIDTH-1:0] tx_fe_q,

   // From TX control
   input [31:0] sample,
   input run,
   output strobe,
   output [31:0] debug
   );

   genvar 	 i;


   wire [17:0] scale_factor;
   wire [31:0] phase_inc;
   reg [31:0]  phase;
   wire [7:0]  interp_rate;
   wire [3:0]  tx_femux_a, tx_femux_b;
   wire        enable_hb1, enable_hb2;
   wire        rate_change;

   setting_reg #(.my_addr(BASE+0)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_inc),.changed());

   setting_reg #(.my_addr(BASE+1), .width(18)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(scale_factor),.changed());

   setting_reg #(.my_addr(BASE+2), .width(10)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({enable_hb1, enable_hb2, interp_rate}),.changed(rate_change));

   // Strobes are all now delayed by 1 cycle for timing reasons
   wire        strobe_cic_pre, strobe_hb1_pre, strobe_hb2_pre;
   reg 	       strobe_cic = 1;
   reg 	       strobe_hb1 = 1;
   reg 	       strobe_hb2 = 1;

  assign strobe = strobe_hb1;

   cic_strober #(.WIDTH(8))
     cic_strober(.clock(clk),.reset(rst),.enable(run & ~rate_change),.rate(interp_rate),
		 .strobe_fast(1'b1),.strobe_slow(strobe_cic_pre) );
   cic_strober #(.WIDTH(2))
     hb2_strober(.clock(clk),.reset(rst),.enable(run & ~rate_change),.rate(enable_hb2 ? 2'd2 : 2'd1),
		 .strobe_fast(strobe_cic_pre),.strobe_slow(strobe_hb2_pre) );
   cic_strober #(.WIDTH(2))
     hb1_strober(.clock(clk),.reset(rst),.enable(run & ~rate_change),.rate(enable_hb1 ? 2'd2 : 2'd1),
		 .strobe_fast(strobe_hb2_pre),.strobe_slow(strobe_hb1_pre) );

   always @(posedge clk) strobe_hb1 <= strobe_hb1_pre;
   always @(posedge clk) strobe_hb2 <= strobe_hb2_pre;
   always @(posedge clk) strobe_cic <= strobe_cic_pre;

   // NCO
   always @(posedge clk)
     if(rst)
       phase <= 0;
     else if(~run)
       phase <= 0;
     else
       phase <= phase + phase_inc;

   wire        signed [17:0] da, db;
   wire        signed [35:0] prod_i, prod_q;

   wire [17:0] i_interp, q_interp;

   wire [17:0] hb1_i, hb1_q, hb2_i, hb2_q;

   wire [7:0]  cpo = enable_hb2 ? ({interp_rate,1'b0}) : interp_rate;
   // Note that max CIC rate is 128, which would give an overflow on cpo if enable_hb2 is true,
   //   but the default case inside hb_interp handles this
   generate
      if (NEW_HB_INTERP == 1) begin: new_hb
	 // First stage of halfband interpolation filters. These run at a max CPO of 2 when CIC is bypassed and HB2 enabled.
	 hb47_int
	   #(.WIDTH(18),
	     .DEVICE(DEVICE))
	     hb1_i0
	       (
		.clk(clk),
		.rst(rst),
		.bypass(~enable_hb1),
		.stb_in(strobe_hb1),
		.data_in({sample[31:16],2'b00}),
		.output_rate(cpo),
		.stb_out(strobe_hb2),
		.data_out(hb1_i)
		);

	 hb47_int
	   #(.WIDTH(18),
	     .DEVICE(DEVICE))
	     hb1_q0
	       (
		.clk(clk),
		.rst(rst),
		.bypass(~enable_hb1),
		.stb_in(strobe_hb1),
		.data_in({sample[15:0],2'b00}),
		.output_rate(cpo),
		.stb_out(strobe_hb2),
		.data_out(hb1_q)
		);

	 // Second stage of halfband interpolation filters. These run at a max CPO of 1 when CIC is bypassed.
	 hb47_int
	   #(.WIDTH(18),
	     .DEVICE(DEVICE))
	     hb2_i0
	       (
		.clk(clk),
		.rst(rst),
		.bypass(~enable_hb2),
		.stb_in(strobe_hb2),
		.data_in(hb1_i),
		.output_rate(interp_rate),
		.stb_out(strobe_cic),
		.data_out(hb2_i)
		);

	 hb47_int
	   #(.WIDTH(18),
	     .DEVICE(DEVICE))
	     hb2_q0
	       (
		.clk(clk),
		.rst(rst),
		.bypass(~enable_hb2),
		.stb_in(strobe_hb2),
		.data_in(hb1_q),
		.output_rate(interp_rate),
		.stb_out(strobe_cic),
		.data_out(hb2_q)
		);

      end else begin: old_hb

	 hb_interp #(.IWIDTH(18),.OWIDTH(18),.ACCWIDTH(WIDTH)) hb_interp_i
	   (.clk(clk),.rst(rst),.bypass(~enable_hb1),.cpo(cpo),.stb_in(strobe_hb1),.data_in({sample[31:16], 2'b0}),.stb_out(strobe_hb2),.data_out(hb1_i));
	 hb_interp #(.IWIDTH(18),.OWIDTH(18),.ACCWIDTH(WIDTH)) hb_interp_q
	   (.clk(clk),.rst(rst),.bypass(~enable_hb1),.cpo(cpo),.stb_in(strobe_hb1),.data_in({sample[15:0], 2'b0}),.stb_out(strobe_hb2),.data_out(hb1_q));

	 small_hb_int #(.WIDTH(18)) small_hb_interp_i
	   (.clk(clk),.rst(rst),.bypass(~enable_hb2),.stb_in(strobe_hb2),.data_in(hb1_i),
	    .output_rate(interp_rate),.stb_out(strobe_cic),.data_out(hb2_i));
	 small_hb_int #(.WIDTH(18)) small_hb_interp_q
	   (.clk(clk),.rst(rst),.bypass(~enable_hb2),.stb_in(strobe_hb2),.data_in(hb1_q),
	    .output_rate(interp_rate),.stb_out(strobe_cic),.data_out(hb2_q));

      end // block: old_hb
   endgenerate

   cic_interp  #(.bw(18),.N(4),.log2_of_max_rate(7))
     cic_interp_i(.clock(clk),.reset(rst),.enable(run & ~rate_change),.rate(interp_rate),
		  .strobe_in(strobe_cic),.strobe_out(1'd1),
		  .signal_in(hb2_i),.signal_out(i_interp));

   cic_interp  #(.bw(18),.N(4),.log2_of_max_rate(7))
     cic_interp_q(.clock(clk),.reset(rst),.enable(run & ~rate_change),.rate(interp_rate),
		  .strobe_in(strobe_cic),.strobe_out(1'd1),
		  .signal_in(hb2_q),.signal_out(q_interp));

   localparam  cwidth = WIDTH;  // was 18
   localparam  zwidth = 24;  // was 16

   wire [cwidth-1:0] da_c, db_c;
   //
   // Note. No head room has been added to the CORDIC to accomodate gain in excess of the input signals dynamic range.
   // The CORDIC has algorithmic gain of 1.647, implementation gain of 0.5 and potential gain associated with rotation of 1.414.
   // Thus the CORDIC will overflow when rotating and an input CW with (clipped) effective amplitude of 1.22 is applied.
   //
   cordic_z24 #(.bitwidth(cwidth))
     cordic(.clock(clk), .reset(rst), .enable(run),
	    .xi({i_interp,{(cwidth-18){1'b0}}}),.yi({q_interp,{(cwidth-18){1'b0}}}),
	    .zi(phase[31:32-zwidth]),
	    .xo(da_c),.yo(db_c),.zo() );

   MULT_MACRO #(.DEVICE(DEVICE),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(18),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
   mult_i (.P(prod_i),             // Multiplier output bus, width determined by WIDTH_P parameter
	   .A(da_c[cwidth-1:cwidth-18]),// Multiplier input A bus, width determined by WIDTH_A parameter
	   .B(scale_factor),       // Multiplier input B bus, width determined by WIDTH_B parameter
	   .CE(1'b1),              // 1-bit active high input clock enable
	   .CLK(clk),              // 1-bit positive edge clock input
	   .RST(rst));             // 1-bit input active high reset

   MULT_MACRO #(.DEVICE(DEVICE),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES"
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(18),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
   mult_q (.P(prod_q),             // Multiplier output bus, width determined by WIDTH_P parameter
	   .A(db_c[cwidth-1:cwidth-18]),// Multiplier input A bus, width determined by WIDTH_A parameter
	   .B(scale_factor),       // Multiplier input B bus, width determined by WIDTH_B parameter
	   .CE(1'b1),              // 1-bit active high input clock enable
	   .CLK(clk),              // 1-bit positive edge clock input
	   .RST(rst));             // 1-bit input active high reset


   wire [32:0] 	     i_clip, q_clip;

   // Cordic rotation coupled with a saturated input signal can cause overflow
   // so we clip here rather than allow a wrap.
   clip_reg #(.bits_in(36), .bits_out(33), .STROBED(1)) clip_i
     (.clk(clk), .in(prod_i[35:0]), .strobe_in(1'b1), .out(i_clip), .strobe_out());
   clip_reg #(.bits_in(36), .bits_out(33), .STROBED(1)) clip_q
     (.clk(clk), .in(prod_q[35:0]), .strobe_in(1'b1), .out(q_clip), .strobe_out());

   assign tx_fe_i = i_clip[32:33-WIDTH];
   assign tx_fe_q = q_clip[32:33-WIDTH];


   //
   // Debug
   //
   assign 	     debug = {strobe_cic, strobe_hb1, strobe_hb2,run};

endmodule // duc_chain
