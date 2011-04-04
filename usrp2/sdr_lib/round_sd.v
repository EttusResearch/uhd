

module round_sd
  #(parameter WIDTH_IN=18,
    parameter WIDTH_OUT=16)
   (input clk, input reset,
    input [WIDTH_IN-1:0] in, output [WIDTH_OUT-1:0] out);

   localparam SUM_WIDTH = WIDTH_IN+1;
   localparam ERR_WIDTH = SUM_WIDTH - (WIDTH_OUT + 1) + 1;
   localparam ACC_WIDTH = ERR_WIDTH + 1;
   
   reg [ACC_WIDTH-1:0] 	 acc;
   wire [SUM_WIDTH-1:0] acc_ext, in_ext;

   sign_extend #(.bits_in(WIDTH_IN),.bits_out(SUM_WIDTH)) ext_in (.in(in), .out(in_ext));
   sign_extend #(.bits_in(ACC_WIDTH),.bits_out(SUM_WIDTH)) ext_acc (.in(acc), .out(acc_ext));
   
   wire [SUM_WIDTH-1:0] sum = in_ext + acc_ext;
   wire [WIDTH_OUT:0] 	sum_round;
   wire [ERR_WIDTH-1:0] err;
   wire [ACC_WIDTH-1:0] err_ext;
   
   //round_reg #(.bits_in(SUM_WIDTH),.bits_out(WIDTH_OUT+1))  round_sum (.clk(clk), .in(sum), .out(sum_round));
   round #(.bits_in(SUM_WIDTH),.bits_out(WIDTH_OUT+1))  round_sum ( .in(sum), .out(sum_round));

   reg [WIDTH_IN-1:0] 	in_del;
   always @(posedge clk)
     in_del <= in;
   
   assign err = in_del - {sum_round,{SUM_WIDTH-WIDTH_OUT-1{1'b0}}};
   
   clip #(.bits_in(WIDTH_OUT+1),.bits_out(WIDTH_OUT)) clip (.in(sum_round), .out(out));

   sign_extend #(.bits_in(ERR_WIDTH),.bits_out(ACC_WIDTH)) ext_err (.in(err), .out(err_ext));
   
   always @(posedge clk)
     if(reset)
       acc <= 0;
     else
       acc <= acc + err_ext;
   
endmodule // rx_dcoffset
