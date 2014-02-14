//
// Copyright 2011-2013 Ettus Research LLC
//


//! The USRP digital down-conversion chain

module ddc_chain
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
   wire [WIDTH-1:0] i_hb1, q_hb1;
   wire [WIDTH-1:0] i_hb2, q_hb2;
   
   wire        strobe_cic, strobe_hb1, strobe_hb2;
   wire        enable_hb1, enable_hb2;
   wire [7:0]  cic_decim_rate;

   reg [WIDTH-1:0]  rx_fe_i_mux, rx_fe_q_mux;
   wire        realmode;
   wire        swap_iq;
   
   setting_reg #(.my_addr(BASE+0)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_inc),.changed());

   setting_reg #(.my_addr(BASE+1), .width(18)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(scale_factor),.changed());

   setting_reg #(.my_addr(BASE+2), .width(10)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({enable_hb1, enable_hb2, cic_decim_rate}),.changed());

   setting_reg #(.my_addr(BASE+3), .width(2)) sr_3
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({realmode,swap_iq}),.changed());

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

   clip_reg #(.bits_in(cwidth), .bits_out(WIDTH)) clip_i
     (.clk(clk), .in(i_cordic), .strobe_in(1'b1), .out(i_cordic_clip));
   clip_reg #(.bits_in(cwidth), .bits_out(WIDTH)) clip_q
     (.clk(clk), .in(q_cordic), .strobe_in(1'b1), .out(q_cordic_clip));

   // CIC decimator  24 bit I/O
   cic_strober cic_strober(.clock(clk),.reset(rst),.enable(run),.rate(cic_decim_rate),
			   .strobe_fast(1),.strobe_slow(strobe_cic) );

   cic_decim #(.bw(WIDTH))
     decim_i (.clock(clk),.reset(rst),.enable(run),
	      .rate(cic_decim_rate),.strobe_in(1'b1),.strobe_out(strobe_cic),
	      .signal_in(i_cordic_clip),.signal_out(i_cic));
   
   cic_decim #(.bw(WIDTH))
     decim_q (.clock(clk),.reset(rst),.enable(run),
	      .rate(cic_decim_rate),.strobe_in(1'b1),.strobe_out(strobe_cic),
	      .signal_in(q_cordic_clip),.signal_out(q_cic));

   // First (small) halfband  24 bit I/O
   small_hb_dec #(.WIDTH(WIDTH)) small_hb_i
     (.clk(clk),.rst(rst),.bypass(~enable_hb1),.run(run),
      .stb_in(strobe_cic),.data_in(i_cic),.stb_out(strobe_hb1),.data_out(i_hb1));
   
   small_hb_dec #(.WIDTH(WIDTH)) small_hb_q
     (.clk(clk),.rst(rst),.bypass(~enable_hb1),.run(run),
      .stb_in(strobe_cic),.data_in(q_cic),.stb_out(),.data_out(q_hb1));

   // Second (large) halfband  24 bit I/O
   wire [8:0]  cpi_hb = enable_hb1 ? {cic_decim_rate,1'b0} : {1'b0,cic_decim_rate};
   hb_dec #(.WIDTH(WIDTH)) hb_i
     (.clk(clk),.rst(rst),.bypass(~enable_hb2),.run(run),.cpi(cpi_hb),
      .stb_in(strobe_hb1),.data_in(i_hb1),.stb_out(strobe_hb2),.data_out(i_hb2));

   hb_dec #(.WIDTH(WIDTH)) hb_q
     (.clk(clk),.rst(rst),.bypass(~enable_hb2),.run(run),.cpi(cpi_hb),
      .stb_in(strobe_hb1),.data_in(q_hb1),.stb_out(),.data_out(q_hb2));

   //scalar operation (gain of 6 bits)
   wire [35:0] prod_i, prod_q;

   MULT18X18S mult_i
     (.P(prod_i), .A(i_hb2[WIDTH-1:WIDTH-18]), .B(scale_factor), .C(clk), .CE(strobe_hb2), .R(rst) );
   MULT18X18S mult_q
     (.P(prod_q), .A(q_hb2[WIDTH-1:WIDTH-18]), .B(scale_factor), .C(clk), .CE(strobe_hb2), .R(rst) );

   //pipeline for the multiplier (gain of 10 bits)
   reg [WIDTH-1:0] prod_reg_i, prod_reg_q;
   reg strobe_mult;

   always @(posedge clk) begin
       strobe_mult <= strobe_hb2;
       prod_reg_i <= prod_i[33:34-WIDTH];
       prod_reg_q <= prod_q[33:34-WIDTH];
   end

   // Round final answer to 16 bits
   round_sd #(.WIDTH_IN(WIDTH),.WIDTH_OUT(16)) round_i
     (.clk(clk),.reset(rst), .in(prod_reg_i),.strobe_in(strobe_mult), .out(sample[31:16]), .strobe_out(strobe));

   round_sd #(.WIDTH_IN(WIDTH),.WIDTH_OUT(16)) round_q
     (.clk(clk),.reset(rst), .in(prod_reg_q),.strobe_in(strobe_mult), .out(sample[15:0]), .strobe_out());

   assign debug = {enable_hb1, enable_hb2, run, strobe, strobe_cic, strobe_hb1, strobe_hb2};
   
endmodule // ddc_chain
