
module round_sd_tb();
   
   reg clk, rst;

   initial rst = 1;
   initial #1000 rst = 0;
   initial clk = 0;
   always #5 clk = ~clk;
   
   initial $dumpfile("round_sd_tb.vcd");
   initial $dumpvars(0,round_sd_tb);

   localparam WIDTH_IN = 8;
   localparam WIDTH_OUT = 5;
   
   reg [WIDTH_IN-1:0] adc_in, adc_in_del;
   wire [WIDTH_OUT-1:0] adc_out;

   integer 		factor = 1<<(WIDTH_IN-WIDTH_OUT);
   
   always @(posedge clk)
     if(~rst)
	begin  
	   if(adc_in_del[WIDTH_IN-1])
	     $write("-%d\t",-adc_in_del);
	   else
	     $write("%d\t",adc_in_del);
	   if(adc_out[WIDTH_OUT-1])
	     $write("-%d\t",-adc_out);
	   else
	     $write("%d\t",adc_out);
	   $write("\n");
	   
	   //$write("%f\t",adc_in_del/factor);
	   //$write("%f\n",adc_in_del/factor-adc_out);
	end	
   
   round_sd #(.WIDTH_IN(WIDTH_IN),.WIDTH_OUT(WIDTH_OUT)) 
   round_sd(.clk(clk),.reset(rst), .in(adc_in), .strobe_in(1'b1), .out(adc_out), .strobe_out());

   reg [5:0] counter = 0;
   
   always @(posedge clk)
     counter <= counter+1;
   
   always @(posedge clk)
     adc_in_del <= adc_in;

   always @(posedge clk)
     if(rst)
       adc_in <= 0;
     else if(counter == 63)
       adc_in <= adc_in + 1;
   
   initial #300000 $finish;
   
endmodule // longfifo_tb
