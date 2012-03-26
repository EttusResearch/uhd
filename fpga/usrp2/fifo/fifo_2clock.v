//
// Copyright 2011-2012 Ettus Research LLC
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


// FIXME ignores the AWIDTH (fifo size) parameter

module fifo_2clock
  #(parameter WIDTH=36, SIZE=6)
   (input wclk, input [WIDTH-1:0] datain, input src_rdy_i, output dst_rdy_o, output [15:0] space,
    input rclk, output [WIDTH-1:0] dataout, output src_rdy_o, input dst_rdy_i, output [15:0] occupied,
    input arst);
   
   wire [SIZE:0] level_rclk, level_wclk; // xilinx adds an extra bit if you ask for accurate levels
   wire 	 full, empty, write, read;

   assign dst_rdy_o  = ~full;
   assign src_rdy_o  = ~empty;
   assign write      = src_rdy_i & dst_rdy_o;
   assign read 	     = src_rdy_o & dst_rdy_i;

   generate
      if((WIDTH <= 36) && (WIDTH > 19)) begin
	wire [35:0] data_in_wide, data_out_wide;
	assign data_in_wide[WIDTH-1:0] = datain;
	assign dataout = data_out_wide[WIDTH-1:0];
	if(SIZE==9)
	  fifo_xlnx_512x36_2clk fifo_xlnx_512x36_2clk
	       (.rst(arst),
		.wr_clk(wclk),.din(data_in_wide),.full(full),.wr_en(write),.wr_data_count(level_wclk),
		.rd_clk(rclk),.dout(data_out_wide),.empty(empty),.rd_en(read),.rd_data_count(level_rclk) );
	else if(SIZE==11)
	  fifo_xlnx_2Kx36_2clk fifo_xlnx_2Kx36_2clk 
		     (.rst(arst),
		      .wr_clk(wclk),.din(data_in_wide),.full(full),.wr_en(write),.wr_data_count(level_wclk),
		      .rd_clk(rclk),.dout(data_out_wide),.empty(empty),.rd_en(read),.rd_data_count(level_rclk) );
	else if(SIZE==6)
	  fifo_xlnx_64x36_2clk fifo_xlnx_64x36_2clk 
		     (.rst(arst),
		      .wr_clk(wclk),.din(data_in_wide),.full(full),.wr_en(write),.wr_data_count(level_wclk),
		      .rd_clk(rclk),.dout(data_out_wide),.empty(empty),.rd_en(read),.rd_data_count(level_rclk) );
	else
	  fifo_xlnx_512x36_2clk fifo_xlnx_512x36_2clk
	       (.rst(arst),
		.wr_clk(wclk),.din(data_in_wide),.full(full),.wr_en(write),.wr_data_count(level_wclk),
		.rd_clk(rclk),.dout(data_out_wide),.empty(empty),.rd_en(read),.rd_data_count(level_rclk) );
      end
      else if((WIDTH <= 19) && (SIZE <= 4)) begin
	wire [18:0] data_in_wide, data_out_wide;
	assign data_in_wide[WIDTH-1:0] = datain;
	assign dataout = data_out_wide[WIDTH-1:0];
	fifo_xlnx_16x19_2clk fifo_xlnx_16x19_2clk
	  (.rst(arst),
	   .wr_clk(wclk),.din(data_in_wide),.full(full),.wr_en(write),.wr_data_count(level_wclk),
	   .rd_clk(rclk),.dout(data_out_wide),.empty(empty),.rd_en(read),.rd_data_count(level_rclk) );
      end
   endgenerate

   assign occupied  = {{(16-SIZE-1){1'b0}},level_rclk};
   assign space     = ((1<<SIZE)+1)-level_wclk;
   
endmodule // fifo_2clock

/*
`else
   // ISE sucks, so the following doesn't work properly

   reg [AWIDTH-1:0] wr_addr, rd_addr;
   wire [AWIDTH-1:0] wr_addr_rclk, rd_addr_wclk;
   wire [AWIDTH-1:0] next_rd_addr;
   wire 	    enb_read;
   
   // Write side management
   wire [AWIDTH-1:0] next_wr_addr = wr_addr + 1;
   always @(posedge wclk or posedge arst)
     if(arst)
       wr_addr <= 0;
     else if(write)
       wr_addr <= next_wr_addr;
   assign 	    full = (next_wr_addr == rd_addr_wclk);

   //  RAM for data storage.  Data out is registered, complicating the
   //     read side logic
   ram_2port #(.DWIDTH(DWIDTH),.AWIDTH(AWIDTH)) mac_rx_ff_ram
     (.clka(wclk),.ena(1'b1),.wea(write),.addra(wr_addr),.dia(datain),.doa(),
      .clkb(rclk),.enb(enb_read),.web(1'b0),.addrb(next_rd_addr),.dib(0),.dob(dataout) );

   // Read side management
   reg 		    data_valid;
   assign 	    empty = ~data_valid;
   assign 	    next_rd_addr = rd_addr + data_valid;
   assign 	    enb_read = read | ~data_valid;

   always @(posedge rclk or posedge arst)
     if(arst)
       rd_addr <= 0;
     else if(read)
       rd_addr <= rd_addr + 1;

   always @(posedge rclk or posedge arst)
     if(arst)
       data_valid <= 0;
     else
       if(read & (next_rd_addr == wr_addr_rclk))
	 data_valid <= 0;
       else if(next_rd_addr != wr_addr_rclk)
	 data_valid <= 1;
	 
   // Send pointers across clock domains via gray code
   gray_send #(.WIDTH(AWIDTH)) send_wr_addr
     (.clk_in(wclk),.addr_in(wr_addr),
      .clk_out(rclk),.addr_out(wr_addr_rclk) );
   
   gray_send #(.WIDTH(AWIDTH)) send_rd_addr
     (.clk_in(rclk),.addr_in(rd_addr),
      .clk_out(wclk),.addr_out(rd_addr_wclk) );

   // Generate fullness info, these are approximate and may be delayed 
   // and are only for higher-level flow control.  
   // Only full and empty are guaranteed exact.
   always @(posedge wclk) 
     level_wclk <= wr_addr - rd_addr_wclk;
   always @(posedge rclk) 
     level_rclk <= wr_addr_rclk - rd_addr;
`endif
endmodule // fifo_2clock

*/
