
module rx_frontend
  #(parameter BASE = 0,
    parameter IQCOMP_EN = 1)
   (input clk, input rst,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    input [15:0] adc_a, input adc_ovf_a,
    input [15:0] adc_b, input adc_ovf_b,

    output [23:0] i_out, output [23:0] q_out,
    input run,
    output [31:0] debug
    );
   
   reg [15:0] 	  adc_i, adc_q;
   wire [17:0] 	  adc_i_ofs, adc_q_ofs;
   wire [35:0] 	  corr_i, corr_q; wire [17:0] 	  mag_corr,phase_corr;
   wire 	  swap_iq;
   
   setting_reg #(.my_addr(BASE), .width(1)) sr_8
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(swap_iq),.changed());

   always @(posedge clk)
     if(swap_iq) // Swap
       {adc_i,adc_q} <= {adc_b,adc_a};
     else
       {adc_i,adc_q} <= {adc_a,adc_b};
       
   setting_reg #(.my_addr(BASE+1),.width(18)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(mag_corr),.changed());
   
   setting_reg #(.my_addr(BASE+2),.width(18)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_corr),.changed());

   generate
      if(IQCOMP_EN == 1)
	begin
	   reg [17:0] adc_i_ofs_dly, adc_q_ofs_dly;
	   always @(posedge clk) begin
	     adc_i_ofs_dly <= adc_i_ofs;
	     adc_q_ofs_dly <= adc_q_ofs;
	   end

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
	      .in1({adc_i_ofs_dly,6'd0}), .in2(corr_i[35:12]), .strobe_in(1'b1),
	      .sum(i_out), .strobe_out());
	   
	   add2_and_clip_reg #(.WIDTH(24)) add_clip_q
	     (.clk(clk), .rst(rst), 
	      .in1({adc_q_ofs_dly,6'd0}), .in2(corr_q[35:12]), .strobe_in(1'b1),
	      .sum(q_out), .strobe_out());
	end // if (IQCOMP_EN == 1)
      else
	begin
	   rx_dcoffset #(.WIDTH(24),.ADDR(BASE+3)) rx_dcoffset_i
	     (.clk(clk),.rst(rst),.set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
	      .in({adc_i,8'b00}),.out(i_out));
	   
	   rx_dcoffset #(.WIDTH(24),.ADDR(BASE+4)) rx_dcoffset_q
	     (.clk(clk),.rst(rst),.set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
	      .in({adc_q,8'b00}),.out(q_out));
	end // else: !if(IQCOMP_EN == 1)
      endgenerate
   
endmodule // rx_frontend
