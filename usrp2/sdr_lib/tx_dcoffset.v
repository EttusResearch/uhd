
// TX DC offset.  Setting is 8 fractional bits, 8 integer bits

module tx_dcoffset 
  #(parameter WIDTH_IN=16,
    parameter WIDTH_OUT=16,
    parameter ADDR=8'd0)
   (input clk, input rst, 
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [WIDTH_IN-1:0] in, output [WIDTH_OUT-1:0] out);

   wire [15:0] 		 dco;
   wire [WIDTH_IN+8-1:0] dco_ext, sum;
 
   setting_reg #(.my_addr(ADDR),.width(16)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),.in(set_data),.out(dco));
   
   sign_extend #(.bits_in(16),.bits_out(WIDTH_IN+8)) ext_err (.in(dco), .out(dco_ext));
 
   add2_and_clip_reg #(.WIDTH(WIDTH_IN+8)) add2_and_clip_reg
     (.clk(clk), .rst(rst), .in1({in,8'd0}), .in2(dco_ext), .strobe_in(1'b1), .sum(sum), .strobe_out());

   round_sd #(.WIDTH_IN(WIDTH_IN+8),.WIDTH_OUT(WIDTH_OUT)) round_sd
     (.clk(clk), .reset(rst), .in(sum), .strobe_in(1'b1), .out(out), .strobe_out());
   
endmodule // rx_dcoffset
