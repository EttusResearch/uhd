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

module sd_spi
  (input clk,
   input rst,
   
   // SD Card interface
   output reg sd_clk,
   output sd_mosi,
   input sd_miso,
   
   // Controls
   input [7:0] clk_div,
   input [7:0] send_dat,
   output [7:0] rcv_dat,
   input go,
   output ready);

   reg [7:0] clk_ctr;   
   reg [3:0] bit_ctr;

   wire      bit_ready = (clk_ctr == 8'd0);
   wire      bit_busy = (clk_ctr != 8'd0);
   wire      bit_done = (clk_ctr == clk_div);

   wire      send_clk_hi = (clk_ctr == (clk_div>>1));
   wire      latch_dat = (clk_ctr == (clk_div - 8'd2));     
   wire      send_clk_lo = (clk_ctr == (clk_div - 8'd1));

   wire      send_bit = (bit_ready && (bit_ctr != 0));
   assign    ready = (bit_ctr == 0);
   
   always @(posedge clk)
     if(rst)
       clk_ctr <= 0;
     else if(bit_done)
       clk_ctr <= 0;
     else if(bit_busy)
       clk_ctr <= clk_ctr + 1;
     else if(send_bit)
       clk_ctr <= 1;

   always @(posedge clk)
     if(rst)
       sd_clk <= 0;
     else if(send_clk_hi)
       sd_clk <= 1;
     else if(send_clk_lo)
       sd_clk <= 0;
   
   always @(posedge clk)
     if(rst)
       bit_ctr <= 0;
     else if(bit_done)
       if(bit_ctr == 4'd8)
	 bit_ctr <= 0;
       else
	 bit_ctr <= bit_ctr + 1;
     else if(bit_ready & go)
       bit_ctr <= 1;
   
   reg [7:0] shift_reg;
   always @(posedge clk)
     if(go)
       shift_reg <= send_dat;
     else if(latch_dat)
       shift_reg <= {shift_reg[6:0],sd_miso};

   assign    sd_mosi = shift_reg[7];
   assign    rcv_dat = shift_reg;
   
endmodule // sd_spi
