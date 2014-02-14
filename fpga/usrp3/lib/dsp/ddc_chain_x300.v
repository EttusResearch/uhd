//
// Copyright 2011-2013 Ettus Research LLC
//

//! X300/X310 digital down-conversion chain

module ddc_chain_x300
  #(
    parameter BASE = 0,
    parameter DSPNO = 0,
    parameter WIDTH = 24
  )
  (input clk, input rst, input clr,
   input set_stb, input [7:0] set_addr, input [31:0] set_data,

   // From RX frontend
   input [WIDTH-1:0] rx_fe_i,
   input [WIDTH-1:0] rx_fe_q,

   // To RX control
   output [31:0] sample,
   input run,
   output strobe,
   output [31:0] debug
   );

   localparam  cwidth = 25;
   localparam  zwidth = 24;

   wire [31:0] phase_inc;
   reg [31:0]  phase;

   wire [17:0] scale_factor;
   wire [cwidth-1:0] i_cordic, q_cordic;
   wire [WIDTH-1:0] i_cordic_clip, q_cordic_clip;
   wire [WIDTH-1:0] i_cic, q_cic;
   wire [46:0] 	    i_hb1, q_hb1;
   wire [46:0] 	    i_hb2, q_hb2;
   wire [47:0] 	    i_hb3, q_hb3;
   
   wire        strobe_cic, strobe_hb1, strobe_hb2, strobe_hb3;

   wire [7:0]  cic_decim_rate;

   reg [WIDTH-1:0]  rx_fe_i_mux, rx_fe_q_mux;
   wire        realmode;
   wire        swap_iq;
   wire [1:0]  hb_rate;
   wire [2:0]  enable_hb = { hb_rate == 2'b11, hb_rate[1] == 1'b1, hb_rate != 2'b00 };
   
   wire        reload_go, reload_we1, reload_we2, reload_we3, reload_ld1, reload_ld2, reload_ld3;
   wire [17:0] coef_din;
   
   setting_reg #(.my_addr(BASE+0)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_inc),.changed());

   setting_reg #(.my_addr(BASE+1), .width(18)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(scale_factor),.changed());

   setting_reg #(.my_addr(BASE+2), .width(10)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({hb_rate, cic_decim_rate}),.changed());

   setting_reg #(.my_addr(BASE+3), .width(2)) sr_3
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({realmode,swap_iq}),.changed());

   setting_reg #(.my_addr(BASE+4), .width(24)) sr_4
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({reload_ld3,reload_we3,reload_ld2,reload_we2,reload_ld1,reload_we1,coef_din}),.changed(reload_go));
   
   // MUX so we can do realmode signals on either input
   
   always @(posedge clk)
     if(swap_iq)
       begin
	  rx_fe_i_mux <= rx_fe_q;
	  rx_fe_q_mux <= realmode ? 0 : rx_fe_i;
       end
     else
       begin
	  rx_fe_i_mux <= rx_fe_i;
	  rx_fe_q_mux <= realmode ? 0 : rx_fe_q;
       end

   // NCO
   always @(posedge clk)
     if(rst)
       phase <= 0;
     else if(~run)
       phase <= 0;
     else
       phase <= phase + phase_inc;

   //sign extension of cordic input
   wire [cwidth-1:0] to_cordic_i, to_cordic_q;
   sign_extend #(.bits_in(WIDTH), .bits_out(cwidth)) sign_extend_cordic_i (.in(rx_fe_i_mux), .out(to_cordic_i));
   sign_extend #(.bits_in(WIDTH), .bits_out(cwidth)) sign_extend_cordic_q (.in(rx_fe_q_mux), .out(to_cordic_q));

   // CORDIC  24-bit I/O
   cordic_z24 #(.bitwidth(cwidth))
     cordic(.clock(clk), .reset(rst), .enable(run),
	    .xi(to_cordic_i),. yi(to_cordic_q), .zi(phase[31:32-zwidth]),
	    .xo(i_cordic),.yo(q_cordic),.zo() );

   clip_reg #(.bits_in(cwidth), .bits_out(WIDTH)) clip_cordic_i
     (.clk(clk), .in(i_cordic), .strobe_in(1'b1), .out(i_cordic_clip));
   clip_reg #(.bits_in(cwidth), .bits_out(WIDTH)) clip_cordic_q
     (.clk(clk), .in(q_cordic), .strobe_in(1'b1), .out(q_cordic_clip));

   // CIC decimator  24 bit I/O
   cic_strober cic_strober(.clock(clk),.reset(rst),.enable(run),.rate(cic_decim_rate),
			   .strobe_fast(1'b1),.strobe_slow(strobe_cic) );

   cic_decim #(.bw(WIDTH))
     decim_i (.clock(clk),.reset(rst),.enable(run),
	      .rate(cic_decim_rate),.strobe_in(1'b1),.strobe_out(strobe_cic),
	      .signal_in(i_cordic_clip),.signal_out(i_cic));
   
   cic_decim #(.bw(WIDTH))
     decim_q (.clock(clk),.reset(rst),.enable(run),
	      .rate(cic_decim_rate),.strobe_in(1'b1),.strobe_out(strobe_cic),
	      .signal_in(q_cordic_clip),.signal_out(q_cic));

   // Halfbands
   wire 	     nd1, nd2, nd3;
   wire 	     rfd1, rfd2, rfd3;
   wire 	     rdy1, rdy2, rdy3;
   wire 	     data_valid1, data_valid2, data_valid3;

   localparam HB1_SCALE = 18;
   localparam HB2_SCALE = 18;
   localparam HB3_SCALE = 18;
   
   assign strobe_hb1 = data_valid1;
   assign strobe_hb2 = data_valid2;
   assign strobe_hb3 = data_valid3;
   assign nd1 = strobe_cic;
   assign nd2 = strobe_hb1;
   assign nd3 = strobe_hb2;
   
   hbdec1 hbdec1
     (.clk(clk), // input clk
      .sclr(rst), // input sclr
      .ce(enable_hb[0]), // input ce
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

   hbdec2 hbdec2
     (.clk(clk), // input clk
      .sclr(rst), // input sclr
      .ce(enable_hb[1]), // input ce
      .coef_ld(reload_go & reload_ld2), // input coef_ld
      .coef_we(reload_go & reload_we2), // input coef_we
      .coef_din(coef_din), // input [17 : 0] coef_din
      .rfd(rfd2), // output rfd
      .nd(nd2), // input nd
      .din_1(i_hb1[23+HB1_SCALE:HB1_SCALE]), // input [23 : 0] din_1
      .din_2(q_hb1[23+HB1_SCALE:HB1_SCALE]), // input [23 : 0] din_2
      .rdy(rdy2), // output rdy
      .data_valid(data_valid2), // output data_valid
      .dout_1(i_hb2), // output [46 : 0] dout_1
      .dout_2(q_hb2)); // output [46 : 0] dout_2

   hbdec3 hbdec3
     (.clk(clk), // input clk
      .sclr(rst), // input sclr
      .ce(enable_hb[2]), // input ce
      .coef_ld(reload_go & reload_ld3), // input coef_ld
      .coef_we(reload_go & reload_we3), // input coef_we
      .coef_din(coef_din), // input [17 : 0] coef_din
      .rfd(rfd3), // output rfd
      .nd(strobe_hb2), // input nd
      .din_1(i_hb2[23+HB2_SCALE:HB2_SCALE]), // input [23 : 0] din_1
      .din_2(q_hb2[23+HB2_SCALE:HB2_SCALE]), // input [23 : 0] din_2
      .rdy(rdy3), // output rdy
      .data_valid(data_valid3), // output data_valid
      .dout_1(i_hb3), // output [47 : 0] dout_1
      .dout_2(q_hb3)); // output [47 : 0] dout_2

   reg [23:0] 	     i_unscaled, q_unscaled;
   reg 		     strobe_unscaled;
   
   always @(posedge clk)
     case(hb_rate)
       2'd0 :
	 begin
	    strobe_unscaled <= strobe_cic;
	    i_unscaled <= i_cic[23:0];
	    q_unscaled <= q_cic[23:0];
	 end
       2'd1 :
	 begin
	    strobe_unscaled <= strobe_hb1;
	    i_unscaled <= i_hb1[23+HB1_SCALE:HB1_SCALE];
	    q_unscaled <= q_hb1[23+HB1_SCALE:HB1_SCALE];
	 end
       2'd2 :
	 begin
	    strobe_unscaled <= strobe_hb2;
	    i_unscaled <= i_hb2[23+HB2_SCALE:HB2_SCALE];
	    q_unscaled <= q_hb2[23+HB2_SCALE:HB2_SCALE];
	 end
       2'd3 :
	 begin
	    strobe_unscaled <= strobe_hb3;
	    i_unscaled <= i_hb3[23+HB3_SCALE:HB3_SCALE];
	    q_unscaled <= q_hb3[23+HB3_SCALE:HB3_SCALE];
	 end
     endcase // case (hb_rate)

   wire [42:0] i_scaled, q_scaled;
   wire [23:0] i_clip, q_clip;
   reg 	       strobe_scaled;
   wire        strobe_clip;
   
   MULT_MACRO #(.DEVICE("7SERIES"),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES" 
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(25),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
   SCALE_I (.P(i_scaled),     // Multiplier output bus, width determined by WIDTH_P parameter 
	    .A({i_unscaled[23],i_unscaled}),     // Multiplier input A bus, width determined by WIDTH_A parameter 
	    .B(scale_factor),                    // Multiplier input B bus, width determined by WIDTH_B parameter 
	    .CE(strobe_unscaled),   // 1-bit active high input clock enable
	    .CLK(clk),              // 1-bit positive edge clock input
	    .RST(rst));             // 1-bit input active high reset
	       
   MULT_MACRO #(.DEVICE("7SERIES"),  // Target Device: "VIRTEX5", "VIRTEX6", "SPARTAN6","7SERIES" 
		.LATENCY(1),         // Desired clock cycle latency, 0-4
		.WIDTH_A(25),        // Multiplier A-input bus width, 1-25
		.WIDTH_B(18))        // Multiplier B-input bus width, 1-18
   SCALE_Q (.P(q_scaled),     // Multiplier output bus, width determined by WIDTH_P parameter 
	    .A({q_unscaled[23],q_unscaled}),     // Multiplier input A bus, width determined by WIDTH_A parameter 
	    .B(scale_factor),                    // Multiplier input B bus, width determined by WIDTH_B parameter 
	    .CE(strobe_unscaled),   // 1-bit active high input clock enable
	    .CLK(clk),              // 1-bit positive edge clock input
	    .RST(rst));             // 1-bit input active high reset

   always @(posedge clk)  strobe_scaled <= strobe_unscaled;
   
   clip_reg #(.bits_in(29), .bits_out(24), .STROBED(1)) clip_i
     (.clk(clk), .in(i_scaled[42:14]), .strobe_in(strobe_scaled), .out(i_clip), .strobe_out(strobe_clip));
   clip_reg #(.bits_in(29), .bits_out(24), .STROBED(1)) clip_q
     (.clk(clk), .in(q_scaled[42:14]), .strobe_in(strobe_scaled), .out(q_clip), .strobe_out());

   round_sd #(.WIDTH_IN(24), .WIDTH_OUT(16)) round_i
     (.clk(clk), .reset(rst), .in(i_clip), .strobe_in(strobe_clip), .out(sample[31:16]), .strobe_out(strobe));
   round_sd #(.WIDTH_IN(24), .WIDTH_OUT(16)) round_q
     (.clk(clk), .reset(rst), .in(q_clip), .strobe_in(strobe_clip), .out(sample[15:0]), .strobe_out());
   
endmodule // ddc_chain
