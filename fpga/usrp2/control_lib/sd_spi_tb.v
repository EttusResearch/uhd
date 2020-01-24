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



module sd_spi_tb;

   reg clk = 0;
   always #5 clk = ~clk;
   reg rst = 1;
   initial #32 rst = 0;

   wire sd_clk, sd_mosi, sd_miso;
   wire [7:0] clk_div = 12;
   wire [7:0] send_dat = 23;
   wire [7:0] rcv_dat;

   wire       ready;
   reg 	      go = 0;
   initial 
     begin
	repeat (100)
	  @(posedge clk);
	go <= 1;
	@(posedge clk);
	go <= 0;
     end
   
   sd_spi dut(.clk(clk),.rst(rst),
	      .sd_clk(sd_clk),.sd_mosi(sd_mosi),.sd_miso(sd_miso),
	      .clk_div(clk_div),.send_dat(send_dat),.rcv_dat(rcv_dat),
	      .go(go),.ready(ready) );

   initial    
     begin 
	$dumpfile("sd_spi_tb.vcd");
	$dumpvars(0,sd_spi_tb);
     end

   initial
     #10000 $finish();
   
endmodule // sd_spi_tb
