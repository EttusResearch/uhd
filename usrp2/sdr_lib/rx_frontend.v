
module rx_frontend
  #(parameter BASE = 0)
   (input clk, input rst,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    input [15:0] adc_a, input adc_ovf_a,
    input [15:0] adc_b, input adc_ovf_b,

    output [17:0] i_out, output [17:0] q_out,
    input run,
    output [31:0] debug
    );
   
   reg [15:0] 	  adc_i, adc_q;
   wire [17:0] 	  adc_i_ofs, adc_q_ofs;
   wire [35:0] 	  corr_i, corr_q;
   wire [17:0] 	  mag_corr,phase_corr;
   wire [7:0] 	  muxctrl;
   wire [23:0] 	  i_final, q_final;
   
   setting_reg #(.my_addr(BASE), .width(8)) sr_8
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(muxctrl),.changed());

   always @(posedge clk)
     case(muxctrl[3:0])		// The I mapping
       0: adc_i <= adc_a;
       1: adc_i <= adc_b;
       2: adc_i <= 0;
       default: adc_i <= 0;
     endcase // case (muxctrl[3:0])
   
   always @(posedge clk)
     case(muxctrl[7:4])		// The Q mapping
       0: adc_q <= adc_a;
       1: adc_q <= adc_b;
       2: adc_q <= 0;
       default: adc_q <= 0;
     endcase // case (muxctrl[7:4])
   
   setting_reg #(.my_addr(BASE+1),.width(18)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(mag_corr),.changed());
   
   setting_reg #(.my_addr(BASE+2),.width(18)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_corr),.changed());
   
   rx_dcoffset #(.WIDTH(18),.ADDR(BASE+3)) rx_dcoffset_i
     (.clk(clk),.rst(rst),.set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .in({adc_i,2'b00}),.out(adc_i_ofs));
   
   rx_dcoffset #(.WIDTH(18),.ADDR(BASE+4)) rx_dcoffset_q
     (.clk(clk),.rst(rst),.set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .in({adc_q,2'b00}),.out(adc_q_ofs));
   
   MULT18X18S mult_mag_corr
     (.P(corr_i), .A(adc_i_ofs), .B(mag_corr), .C(clk), .CE(1), .R(rst) ); 

   MULT18X18S mult_phase_corr
     (.P(corr_q), .A(adc_i_ofs), .B(phase_corr), .C(clk), .CE(1), .R(rst) );
   
   add2_and_clip_reg #(.WIDTH(24)) add_clip_i
     (.clk(clk), .rst(rst), 
      .in1({adc_i_ofs,6'd0}), .in2({{4{corr_i[35]}},corr_i[35:16]}), .sum(i_final));
   
   add2_and_clip_reg #(.WIDTH(24)) add_clip_q
     (.clk(clk), .rst(rst), 
      .in1({adc_q_ofs,6'd0}), .in2({{4{corr_q[35]}},corr_q[35:16]}), .sum(q_final));

   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(18)) round_i (.clk(clk), .reset(rst), .in(i_final), .out(i_out));
   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(18)) round_q (.clk(clk), .reset(rst), .in(q_final), .out(q_out));
   
endmodule // rx_frontend
