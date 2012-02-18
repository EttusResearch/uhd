//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

module double_buffer_tb();
   
   reg clk = 0;
   reg rst = 1;
   reg clear = 0;
   initial #1000 rst = 0;
   always #50 clk = ~clk;

   wire src_rdy_o;
   reg src_rdy_i = 0;
   wire dst_rdy_o;
   
   wire dst_rdy_i = 0;
   wire [35:0] data_o;
   reg [35:0]  data_i;

   wire        access_we, access_stb, access_done, access_ok, access_skip_read;

   wire [8:0]  access_adr, access_len;
   wire [35:0] dsp_to_buf, buf_to_dsp;
   reg 	       set_stb = 0;
   
   double_buffer db
     (.clk(clk),.reset(rst),.clear(0),
      .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
      .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
      .access_dat_i(dsp_to_buf), .access_dat_o(buf_to_dsp),

      .data_i(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o),
      .data_o(data_o), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));

   dspengine_8to16 #(.HEADER_OFFSET(1)) dspengine_8to16
     (.clk(clk),.reset(rst),.clear(0),
      .set_stb(set_stb), .set_addr(0), .set_data(1),
      .access_we(access_we), .access_stb(access_stb), .access_ok(access_ok), .access_done(access_done), 
      .access_skip_read(access_skip_read), .access_adr(access_adr), .access_len(access_len), 
      .access_dat_i(buf_to_dsp), .access_dat_o(dsp_to_buf));
   
   always @(posedge clk)
     if(src_rdy_o & dst_rdy_i)
       begin
	  $display("SOF %d, EOF %d, OCC %x, DAT %x",data_o[32],data_o[33],data_o[35:34],data_o[31:0]);
	  if(data_o[33])
	    $display();
       end	  
   initial $dumpfile("double_buffer_tb.vcd");
   initial $dumpvars(0,double_buffer_tb);

   initial
     begin
	@(negedge rst);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
/*
	// Passthrough
	$display("Passthrough");
	src_rdy_i <= 1;
	data_i <= { 2'b00,1'b0,1'b1,32'h01234567};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'hFFFFFFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h04050607};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h08090a0b};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'h0c0d0e0f};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

	repeat (5)
	  @(posedge clk);
*/
	$display("Enabled");
	set_stb <= 1;
	@(posedge clk);
	set_stb <= 0;
/*
	@(posedge clk);
	$display("Non-IF Data Passthrough");
	src_rdy_i <= 1;
	data_i <= { 2'b00,1'b0,1'b1,32'h89acdef0};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'hC0000000};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h14151617};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h18191a1b};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'h1c1d1e1f};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

	while(~dst_rdy_o)
	  @(posedge clk);
	
	$display("No StreamID, No Trailer, Even");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'hAAAAAAAA};
	@(posedge clk);
  	data_i <= { 2'b00,1'b0,1'b0,32'h0000FFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h01000200};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h03000400};
	src_rdy_i <= 0;
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h05000600};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h07000800};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h09000a00};
  	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'h0b000c00};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

	while(~dst_rdy_o)
	  @(posedge clk);

	$display("No StreamID, No Trailer, Odd");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'hBBBBBBBB};
	@(posedge clk);
  	data_i <= { 2'b00,1'b0,1'b0,32'h0000FFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h11001200};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h13001400};
	src_rdy_i <= 0;
	@(posedge clk);
	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h15001600};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h17001800};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'h19001a00};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

	while(~dst_rdy_o)
	  @(posedge clk);
*/
   /*
	$display("No StreamID, Trailer, Even");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'hCCCCCCCC};
	@(posedge clk);
  	data_i <= { 2'b00,1'b0,1'b0,32'h0400FFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h21222324};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h25262728};
	src_rdy_i <= 0;
	@(posedge clk);
	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h292a2b2c};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h2d2e2f30};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'hDEADBEEF};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);
*/
	while(~dst_rdy_o)
	  @(posedge clk);
/*
	$display("No StreamID, Trailer, Odd");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'hDDDDDDDD};
	@(posedge clk);
  	data_i <= { 2'b00,1'b0,1'b0,32'h0400FFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h21222324};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h25262728};
	src_rdy_i <= 0;
	@(posedge clk);
	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h292a2b2c};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h2d2e2f30};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'hDEBDBF0D};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);
*/
	while(~dst_rdy_o)
	  @(posedge clk);

/*
	$display("No StreamID, Trailer, Odd");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'h0400FFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h31003200};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h33003400};
	src_rdy_i <= 0;
	@(posedge clk);
	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h35003600};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'h39003a00};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

	while(~dst_rdy_o)
	  @(posedge clk);

	$display("StreamID, No Trailer, Even");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'h1000FFFF};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h11001200};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h13001400};
	src_rdy_i <= 0;
	@(posedge clk);
	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h15001600};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'h17001800};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'h19001a00};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

	while(~dst_rdy_o)
	  @(posedge clk);
*/
	$display("StreamID, Trailer, Odd");
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'hABCDEF98};
	@(posedge clk);
  	data_i <= { 2'b00,1'b0,1'b0,32'h1c034567};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'ha0a1a2a3};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'ha4a5a6a7};
//	src_rdy_i <= 0;
//	@(posedge clk);
//	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'ha8a9aaab};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'hacadaeaf};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'hdeadbeef};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);
	src_rdy_i <= 1;
  	data_i <= { 2'b00,1'b0,1'b1,32'hABCDEF98};
	@(posedge clk);
  	data_i <= { 2'b00,1'b0,1'b0,32'h1c034567};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'ha0a1a2a3};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'ha4a5a6a7};
//	src_rdy_i <= 0;
//	@(posedge clk);
//	src_rdy_i <= 1;
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'ha8a9aaab};
	@(posedge clk);
	data_i <= { 2'b00,1'b0,1'b0,32'hacadaeaf};
	@(posedge clk);
	data_i <= { 2'b00,1'b1,1'b0,32'hdeadbeef};
	@(posedge clk);
	src_rdy_i <= 0;
	@(posedge clk);

     end
   
   initial #28000 $finish;
endmodule // double_buffer_tb
