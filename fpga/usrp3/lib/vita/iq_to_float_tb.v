module iq_to_float_tb();


   reg clk, reset;
   integer x,file;
   reg [15:0] in;
   wire [31:0] out;
   
   initial clk = 0;
   
   always #10 clk = ~clk;

   initial $dumpfile("iq_to_float_tb.vcd");
   initial $dumpvars(0,iq_to_float_tb);
   integer f;
   initial
    
     begin
	x <= 0;
	reset <= 1;
	in <= 0;
    file = $fopen("iq_to_float_VER.txt");
	
	repeat(65536) @(posedge clk);
	reset <= 0;
	repeat(65536) @(posedge clk)
	  begin
	     in <= data[x];
	     x  <= x+1;
         $fdisplayh(file,out);
	  end
      $fclose(file);
	
	     
	repeat(65536) @(posedge clk);
	$finish;

     end
   
  
   

    iq_to_float #(.BITS_IN(16), .BITS_OUT(32))
   dut
     (
      .in(in), .out(out), .clk(clk), .reset(reset)
      );

   

   reg 	     [15:0] data [0:65535];
   initial $readmemh("iq_to_float_input.txt",data);

  

	
	
   
   
endmodule

