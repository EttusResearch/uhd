
module tx_frontend
  #(parameter BASE=0,
    parameter WIDTH_OUT=16,
    parameter IQCOMP_EN=1)
   (input clk, input rst,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [23:0] tx_i, input [23:0] tx_q, input run,
    output reg [WIDTH_OUT-1:0] dac_a, output reg [WIDTH_OUT-1:0] dac_b
    );

   // IQ balance --> DC offset --> rounding --> mux

   wire [23:0] i_dco, q_dco, i_ofs, q_ofs;
   wire [WIDTH_OUT-1:0] i_final, q_final;
   wire [7:0]  mux_ctrl;
   wire [35:0] corr_i, corr_q;
   wire [23:0] i_bal, q_bal;
   wire [17:0] mag_corr, phase_corr;
   
   setting_reg #(.my_addr(BASE+0), .width(24)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(i_dco),.changed());

   setting_reg #(.my_addr(BASE+1), .width(24)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(q_dco),.changed());

   setting_reg #(.my_addr(BASE+2),.width(18)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(mag_corr),.changed());
   
   setting_reg #(.my_addr(BASE+3),.width(18)) sr_3
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_corr),.changed());
  
   setting_reg #(.my_addr(BASE+4), .width(8)) sr_4
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(mux_ctrl),.changed());

   generate
      if(IQCOMP_EN==1)
	begin
	   reg [23:0] tx_i_dly, tx_q_dly;
	   always @(posedge clk) begin
	     tx_i_dly <= tx_i;
	     tx_q_dly <= tx_q;
	   end

	   // IQ Balance
	   MULT18X18S mult_mag_corr
	     (.P(corr_i), .A(tx_i[23:6]), .B(mag_corr), .C(clk), .CE(1), .R(rst) ); 
	   
	   MULT18X18S mult_phase_corr
	     (.P(corr_q), .A(tx_i[23:6]), .B(phase_corr), .C(clk), .CE(1), .R(rst) );
	   
	   add2_and_clip_reg #(.WIDTH(24)) add_clip_i
	     (.clk(clk), .rst(rst), 
	      .in1(tx_i_dly), .in2(corr_i[35:12]), .strobe_in(1'b1),
	      .sum(i_bal), .strobe_out());
	   
	   add2_and_clip_reg #(.WIDTH(24)) add_clip_q
	     (.clk(clk), .rst(rst), 
	      .in1(tx_q_dly), .in2(corr_q[35:12]), .strobe_in(1'b1),
	      .sum(q_bal), .strobe_out());

	   // DC Offset
	   add2_and_clip_reg #(.WIDTH(24)) add_dco_i
	     (.clk(clk), .rst(rst), .in1(i_dco), .in2(i_bal), .strobe_in(1'b1), .sum(i_ofs), .strobe_out());
	   
	   add2_and_clip_reg #(.WIDTH(24)) add_dco_q
	     (.clk(clk), .rst(rst), .in1(q_dco), .in2(q_bal), .strobe_in(1'b1), .sum(q_ofs), .strobe_out());
	end // if (IQCOMP_EN==1)
      else
	begin
	   // DC Offset
	   add2_and_clip_reg #(.WIDTH(24)) add_dco_i
	     (.clk(clk), .rst(rst), .in1(i_dco), .in2(tx_i), .strobe_in(1'b1), .sum(i_ofs), .strobe_out());
	   
	   add2_and_clip_reg #(.WIDTH(24)) add_dco_q
	     (.clk(clk), .rst(rst), .in1(q_dco), .in2(tx_q), .strobe_in(1'b1), .sum(q_ofs), .strobe_out());
	end // else: !if(IQCOMP_EN==1)
   endgenerate
   
   // Rounding
   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(WIDTH_OUT)) round_i
     (.clk(clk), .reset(rst), .in(i_ofs),.strobe_in(1'b1), .out(i_final), .strobe_out());

   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(WIDTH_OUT)) round_q
     (.clk(clk), .reset(rst), .in(q_ofs),.strobe_in(1'b1), .out(q_final), .strobe_out());

   // Mux
   always @(posedge clk)
     case(mux_ctrl[3:0])
       0 : dac_a <= i_final;
       1 : dac_a <= q_final;
       default : dac_a <= 0;
     endcase // case (mux_ctrl[3:0])
      
   always @(posedge clk)
     case(mux_ctrl[7:4])
       0 : dac_b <= i_final;
       1 : dac_b <= q_final;
       default : dac_b <= 0;
     endcase // case (mux_ctrl[7:4])
      
endmodule // tx_frontend
