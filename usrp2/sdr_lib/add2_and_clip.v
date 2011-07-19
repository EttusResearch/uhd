
module add2_and_clip
  #(parameter WIDTH=16)
    (input [WIDTH-1:0] in1,
     input [WIDTH-1:0] in2,
     output [WIDTH-1:0] sum);

   wire [WIDTH:0] 	sum_int = {in1[WIDTH-1],in1} + {in2[WIDTH-1],in2};
   clip #(.bits_in(WIDTH+1),.bits_out(WIDTH)) clip
     (.in(sum_int),.out(sum));
   
endmodule // add2_and_clip
