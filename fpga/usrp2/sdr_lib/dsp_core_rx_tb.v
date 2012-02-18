
`timescale 1ns/1ns
module ddc_chain_tb();
   
   reg clk, rst;

   initial rst = 1;
   initial #1000 rst = 0;
   initial clk = 0;
   always #5 clk = ~clk;
   
   initial $dumpfile("ddc_chain_tb.vcd");
   initial $dumpvars(0,ddc_chain_tb);
   
   reg signed [23:0] adc_in;
   wire signed [15:0] adc_out_i, adc_out_q;

   always @(posedge clk)
     begin
	$display(adc_in);
	$display(adc_out_i);
	$display(adc_out_q);
     end	

   reg run;
   reg set_stb;
   reg [7:0] set_addr;
   reg [31:0] set_data;
   
   ddc_chain #(.BASE(0)) ddc_chain
     (.clk(clk),.rst(rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .adc_i(adc_in), .adc_ovf_i(0),
      .adc_q(0), .adc_ovf_q(0),
      .sample({adc_out_i,adc_out_q}),
      .run(run), .strobe(), .debug());

   initial
     begin
	run <= 0;
	@(negedge rst);
	@(posedge clk);
	set_addr <= 1;
	set_data <= {16'd64,16'd64};  // set gains
	set_stb <= 1;
	@(posedge clk);
	set_addr <= 2;
	set_data <= {16'd0,8'd3,8'd1}; // set decim
	set_stb <= 1;
	@(posedge clk);
	set_addr <= 0;
	//set_data <= {32'h0000_0000};
	set_data <= {32'h01CA_C083}; // 700 kHz
	set_stb <= 1;
	@(posedge clk);
	set_stb <= 0;
	@(posedge clk);
	run <= 1;
     end

   always @(posedge clk)
     //adc_in <= 24'd1000000;
     adc_in <= 24'h80_0000;
   
   /*
   always @(posedge clk)
     if(rst)
       adc_in <= 0;
     else
       adc_in <= adc_in + 4;
   //adc_in <= (($random % 473) + 23)/4;
*/   
endmodule // ddc_chain_tb
