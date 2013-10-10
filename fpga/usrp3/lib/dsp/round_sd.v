

module round_sd
  #(parameter WIDTH_IN=18,
    parameter WIDTH_OUT=16,
    parameter DISABLE_SD=0)
   (input clk, input reset,
    input [WIDTH_IN-1:0] in, input strobe_in,
    output [WIDTH_OUT-1:0] out, output strobe_out);

   localparam ERR_WIDTH = WIDTH_IN - WIDTH_OUT + 1;

   wire [ERR_WIDTH-1:0]  err;
   wire [WIDTH_IN-1:0] 	 err_ext, sum;
  
   sign_extend #(.bits_in(ERR_WIDTH),.bits_out(WIDTH_IN)) ext_err (.in(err), .out(err_ext));
   
   add2_and_clip_reg #(.WIDTH(WIDTH_IN)) add2_and_clip_reg
     (.clk(clk), .rst(reset), .in1(in), .in2((DISABLE_SD == 0) ? err_ext : 0), .strobe_in(strobe_in), .sum(sum), .strobe_out(strobe_out));
   
   round #(.bits_in(WIDTH_IN),.bits_out(WIDTH_OUT)) round_sum (.in(sum), .out(out), .err(err));
   
endmodule // round_sd
