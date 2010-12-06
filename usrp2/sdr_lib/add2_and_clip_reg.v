
module add2_and_clip_reg
  #(parameter WIDTH=16)
    (input clk,
     input rst,
     input [WIDTH-1:0] in1,
     input [WIDTH-1:0] in2,
     output reg [WIDTH-1:0] sum);

   wire [WIDTH-1:0] sum_int;
   
   add2_and_clip #(.WIDTH(WIDTH)) add2_and_clip (.in1(in1),.in2(in2),.sum(sum_int));

   always @(posedge clk)
     if(rst)
       sum <= 0;
     else
       sum <= sum_int;
   
endmodule // add2_and_clip_reg
