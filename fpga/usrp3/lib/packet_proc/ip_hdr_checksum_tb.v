
module ip_hdr_checksum_tb();
   
   initial $dumpfile("ip_hdr_checksum_tb.vcd");
   initial $dumpvars(0,ip_hdr_checksum_tb);

   reg clk;
   
   wire [159:0] in = {
		      16'h4500,
		      16'h0030,
		      16'h4422,
		      16'h4000,
		      16'h8006,
		      16'h0000,
		      16'h8c7c,
		      16'h19ac,
		      16'hae24,
		      16'h1e2b
		      };
   
   wire [15:0] 	out;
   ip_hdr_checksum ip_hdr_checksum
     (.clk(clk), 
      .in(in),
      .out(out));

   initial
     begin
	clk <= 0;
	#100 clk <= 1;
	#100 clk <= 0;
	#100 clk <= 1;
	#100 $display("Computed 0x%x, should be 0x442e", out);
	#100 $finish;
     end
   
endmodule // ip_hdr_checksum_tb
