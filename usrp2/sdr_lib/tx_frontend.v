
module tx_frontend
  #(parameter BASE=0)
   (input clk, input rst,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [23:0] tx_i, input [23:0] tx_q, input run,
    output reg [15:0] dac_a, output reg [15:0] dac_b
    );

   // IQ balance --> DC offset --> rounding --> mux

   wire [23:0] i_dco, q_dco, i_ofs, q_ofs;
   wire [15:0] i_final, q_final;
   wire [7:0]  mux_ctrl;
   
   setting_reg #(.my_addr(BASE+0), .width(24)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(i_dco),.changed());

   setting_reg #(.my_addr(BASE+1), .width(24)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(q_dco),.changed());

   setting_reg #(.my_addr(BASE+2), .width(4)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(mux_ctrl),.changed());

   add2_and_clip_reg #(.WIDTH(24)) add_dco_i
     (.clk(clk), .rst(rst), .in1(i_dco), .in2(tx_i), .strobe_in(1'b1), .sum(i_ofs), .strobe_out());
   
   add2_and_clip_reg #(.WIDTH(24)) add_dco_q
     (.clk(clk), .rst(rst), .in1(q_dco), .in2(tx_q), .strobe_in(1'b1), .sum(q_ofs), .strobe_out());
   
   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_i
     (.clk(clk), .reset(rst), .in(i_ofs),.strobe_in(1'b1), .out(i_final), .strobe_out());

   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_q
     (.clk(clk), .reset(rst), .in(q_ofs),.strobe_in(1'b1), .out(q_final), .strobe_out());

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
