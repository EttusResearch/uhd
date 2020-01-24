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


module packet_splitter_tb();
   
   reg sys_clk = 0;
   reg sys_rst = 1;
   reg gpif_clk = 0;
   reg gpif_rst = 1;
      
   reg [15:0] gpif_data;
   reg 	      WR = 0, EP = 0;

   wire       CF, DF;
   
   wire       gpif_full_d, gpif_full_c;
   wire [18:0] data_o, ctrl_o, data_splt;
   wire        src_rdy, dst_rdy, src_rdy_splt, dst_rdy_splt;
   wire        ctrl_src_rdy, ctrl_dst_rdy;

   assign ctrl_dst_rdy = 1;
   
   initial $dumpfile("packet_splitter_tb.vcd");
   initial $dumpvars(0,packet_splitter_tb);

   initial #1000 gpif_rst = 0;
   initial #1000 sys_rst = 0;
   always #64 gpif_clk <= ~gpif_clk;
   always #47.9 sys_clk <= ~sys_clk;

   wire [35:0] data_int;
   wire        src_rdy_int, dst_rdy_int;

   assign dst_rdy_splt = 1;

   vita_pkt_gen vita_pkt_gen
     (.clk(sys_clk), .reset(sys_rst) , .clear(0),
      .len(512),.data_o(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int));

   fifo36_to_fifo19 #(.LE(1)) f36_to_f19
     (.clk(sys_clk), .reset(sys_rst), .clear(0),
      .f36_datain(data_int), .f36_src_rdy_i(src_rdy_int), .f36_dst_rdy_o(dst_rdy_int),
      .f19_dataout(data_o), .f19_src_rdy_o(src_rdy), .f19_dst_rdy_i(dst_rdy));
   		 
   packet_splitter #(.FRAME_LEN(13)) rx_packet_splitter
     (.clk(sys_clk), .reset(sys_rst), .clear(0),
      .frames_per_packet(4),
      .data_i(data_o), .src_rdy_i(src_rdy), .dst_rdy_o(dst_rdy),
      .data_o(data_splt), .src_rdy_o(src_rdy_splt), .dst_rdy_i(dst_rdy_splt));

   always @(posedge sys_clk)
     if(ctrl_src_rdy & ctrl_dst_rdy)
       $display("CTRL: %x",ctrl_o);

   always @(posedge sys_clk)
     if(src_rdy_splt & dst_rdy_splt)
       begin
	  if(data_splt[16])
	    $display("<-------- DATA SOF--------->");
	  $display("DATA: %x",data_splt);
	  if(data_splt[17])
	    $display("<-------- DATA EOF--------->");
       end
   
   initial
     begin
	#10000;
	repeat (1)
	  begin
	     @(posedge gpif_clk);
	     
	     WR <= 1;
	     gpif_data <= 256;  // Length
	     @(posedge gpif_clk);
	     gpif_data <= 16'h00;
	     @(posedge gpif_clk);
	     repeat(254)
	       begin
		  gpif_data <= gpif_data + 1;
		  @(posedge gpif_clk);
	       end
	     WR <= 0;

	     while(DF)
	       @(posedge gpif_clk);
	     repeat (16)
	       @(posedge gpif_clk);
	     
	     WR <= 1;
	     repeat(256)
	       begin
		  gpif_data <= gpif_data - 1;
		  @(posedge gpif_clk);
	       end
	     WR <= 0;


/*
	     while(DF)
	       @(posedge gpif_clk);
	     	     
	     repeat (20)
	       @(posedge gpif_clk);
	     WR <= 1;
	     gpif_data <= 16'h5;
	     @(posedge gpif_clk);
	     gpif_data <= 16'h00;
	     @(posedge gpif_clk);
	     repeat(254)
	       begin
		  gpif_data <= gpif_data - 1;
		  @(posedge gpif_clk);
	       end
	     WR <= 0;
 */
	  end
     end // initial begin
   
   initial #200000 $finish;
   
     
endmodule // packet_splitter_tb
