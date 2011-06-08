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

module fifo_tb();
   
   reg clk = 0;
   reg rst = 1;
   reg clear = 0;
   initial #1000 rst = 0;
   always #50 clk = ~clk;

   reg [18:0] f19a;
   wire [18:0] f19b, f19c, f19d;
   wire [35:0] f36a, f36b;

   reg 	       f19a_sr = 0;
   wire        f19b_sr, f19c_sr, f19d_sr, f36a_sr, f36b_sr;
   wire        f19a_dr, f19b_dr, f19c_dr, f19d_dr, f36a_dr, f36b_dr;
   
   fifo_short #(.WIDTH(19)) fifo_short1
     (.clk(clk),.reset(rst),.clear(clear),
      .datain(f19a),.src_rdy_i(f19a_sr),.dst_rdy_o(f19a_dr),
      .dataout(f19b),.src_rdy_o(f19b_sr),.dst_rdy_i(f19b_dr) );

   fifo19_to_fifo36 fifo19_to_fifo36
     (.clk(clk),.reset(rst),.clear(clear),
      .f19_datain(f19b),.f19_src_rdy_i(f19b_sr),.f19_dst_rdy_o(f19b_dr),
      .f36_dataout(f36a),.f36_src_rdy_o(f36a_sr),.f36_dst_rdy_i(f36a_dr) );

   fifo_short #(.WIDTH(36)) fifo_short2
     (.clk(clk),.reset(rst),.clear(clear),
      .datain(f36a),.src_rdy_i(f36a_sr),.dst_rdy_o(f36a_dr),
      .dataout(f36b),.src_rdy_o(f36b_sr),.dst_rdy_i(f36b_dr) );

   fifo36_to_fifo19 fifo36_to_fifo19
     (.clk(clk),.reset(rst),.clear(clear),
      .f36_datain(f36b),.f36_src_rdy_i(f36b_sr),.f36_dst_rdy_o(f36b_dr),
      .f19_dataout(f19c),.f19_src_rdy_o(f19c_sr),.f19_dst_rdy_i(f19c_dr) );

   fifo_short #(.WIDTH(19)) fifo_short3
     (.clk(clk),.reset(rst),.clear(clear),
      .datain(f19c),.src_rdy_i(f19c_sr),.dst_rdy_o(f19c_dr),
      .dataout(f19d),.src_rdy_o(f19d_sr),.dst_rdy_i(f19d_dr) );

   assign f19d_dr = 1;

    always @(posedge clk)
     if(f19a_sr & f19a_dr)
       $display("18IN: %h", f19a);
  
    always @(posedge clk)
     if(f19d_sr & f19d_dr)
       $display("                            18OUT: %h", f19d);
  
   always @(posedge clk)
     if(f36b_sr & f36b_dr)
       $display("             36: %h", f36b);
   
   initial $dumpfile("fifo_tb.vcd");
   initial $dumpvars(0,fifo_tb);

   initial
     begin
	@(negedge rst);
	@(posedge clk);
	repeat (2)
	  begin
	     f19a <= 19'h1_AA01;
	     f19a_sr <= 1;
	     @(posedge clk);
	     f19a <= 19'h0_AA02;
	     repeat (4)
	       begin
		  @(posedge clk);
		  f19a <= f19a + 1;
	       end
	     f19a[18:16] <= 3'b010;
	     @(posedge clk);
	     f19a_sr <= 0;
	     f19a <= 19'h7_FFFF;
	     @(posedge clk);
	  end
	#20000 $finish;
     end
endmodule // longfifo_tb
