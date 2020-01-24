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

module prot_eng_tx_tb();

   localparam BASE = 128;
   reg clk    = 0;
   reg rst    = 1;
   reg clear  = 0;
   initial #1000 rst = 0;
   always #50 clk = ~clk;
   
   reg [31:0] f36_data;
   reg [1:0]  f36_occ;
   reg 	      f36_sof, f36_eof;
   wire [35:0] f36_in = {f36_occ,f36_eof,f36_sof,f36_data};
   reg 	       src_rdy_f36i  = 0;
   wire        dst_rdy_f36i;


   wire [35:0] casc_do;
   wire        src_rdy_f36o, dst_rdy_f36o;

   wire [35:0] prot_out;
   wire        src_rdy_prot, dst_rdy_prot;

   wire [35:0] realign_out;
   wire        src_rdy_realign;
   reg 	       dst_rdy_realign = 1;
      
   reg [15:0] count;

   reg set_stb;
   reg [7:0] set_addr;
   reg [31:0] set_data;
	
   fifo_short #(.WIDTH(36)) fifo_cascade36
     (.clk(clk),.reset(rst),.clear(clear),
      .datain(f36_in),.src_rdy_i(src_rdy_f36i),.dst_rdy_o(dst_rdy_f36i),
      .dataout(casc_do),.src_rdy_o(src_rdy_f36o),.dst_rdy_i(dst_rdy_f36o));

   prot_eng_tx #(.BASE(BASE)) prot_eng_tx
     (.clk(clk), .reset(rst), .clear(0),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .datain(casc_do),.src_rdy_i(src_rdy_f36o),.dst_rdy_o(dst_rdy_f36o),
      .dataout(prot_out),.src_rdy_o(src_rdy_prot),.dst_rdy_i(dst_rdy_prot));

   ethtx_realign ethtx_realign
     (.clk(clk), .reset(rst), .clear(0),
      .datain(prot_out),.src_rdy_i(src_rdy_prot),.dst_rdy_o(dst_rdy_prot),
      .dataout(realign_out),.src_rdy_o(src_rdy_realign),.dst_rdy_i(dst_rdy_realign));

   reg [35:0] printer;

   task WriteSREG;
      input [7:0] addr;
      input [31:0] data;

      begin
	 @(posedge clk);
	 set_addr <= addr;
	 set_data <= data;
	 set_stb  <= 1;
	 @(posedge clk);
	 set_stb <= 0;
      end
   endtask // WriteSREG
   	
   always @(posedge clk)
     if(src_rdy_realign)
       $display("Read: %h",realign_out);

   
   task ReadFromFIFO36;
      begin
	 $display("Read from FIFO36");
	 #1 dst_rdy_realign <= 1;
	 while(~src_rdy_prot)
	   @(posedge clk);
	 while(1)
	   begin
	      while(~src_rdy_prot)
		@(posedge clk);
	      $display("Read: %h",realign_out);
	      @(posedge clk);
	   end
      end
   endtask // ReadFromFIFO36
   
   task PutPacketInFIFO36;
      input [31:0] data_start;
      input [31:0] data_len;
      begin
	 count 	      <= 4;
	 src_rdy_f36i <= 1;
	 f36_data     <= 32'h0001_000c;
	 f36_sof      <= 1;
	 f36_eof      <= 0;
	 f36_occ      <= 0;
	
	 $display("Put Packet in FIFO36");
	 while(~dst_rdy_f36i)
	   @(posedge clk);
	 @(posedge clk);
	 $display("PPI_FIFO36: Entered First Line");
	 f36_sof  <= 0;
	 f36_data <= data_start;
	 while(~dst_rdy_f36i)
	   @(posedge clk);
	 @(posedge clk);
	 while(count+4 < data_len)
	   begin
	      f36_data <= f36_data + 32'h01010101;
	      count    <= count + 4;
	      while(~dst_rdy_f36i)
		@(posedge clk);
	      @(posedge clk);
	      $display("PPI_FIFO36: Entered New Line");
	   end
	 f36_data  <= f36_data + 32'h01010101;
	 f36_eof   <= 1;
	 if(count + 4 == data_len)
	   f36_occ <= 0;
	 else if(count + 3 == data_len)
	   f36_occ <= 3;
	 else if(count + 2 == data_len)
	   f36_occ <= 2;
	 else
	   f36_occ <= 1;
	 while(~dst_rdy_f36i)
	   @(posedge clk);
	 @(posedge clk);
	 f36_occ      <= 0;
	 f36_eof      <= 0;
	 f36_data     <= 0;
	 src_rdy_f36i <= 0;
	 $display("PPI_FIFO36: Entered Last Line");
      end
   endtask // PutPacketInFIFO36
   
   initial $dumpfile("prot_eng_tx_tb.vcd");
   initial $dumpvars(0,prot_eng_tx_tb);

   initial
     begin
	#10000;
	@(posedge clk);
	//ReadFromFIFO36;
     end
   
   initial
     begin
	@(negedge rst);
	@(posedge clk);
	WriteSREG(BASE, 32'h89AB_CDEF);
	WriteSREG(BASE+1, 32'h1111_2222);
	WriteSREG(BASE+2, 32'h3333_4444);
	WriteSREG(BASE+3, 32'h5555_6666);
	WriteSREG(BASE+4, 32'h7777_8888);
	WriteSREG(BASE+5, 32'h9999_aaaa);
	WriteSREG(BASE+6, 32'hbbbb_cccc);
	WriteSREG(BASE+7, 32'hdddd_eeee);
	WriteSREG(BASE+8, 32'h0f0f_0011);
	WriteSREG(BASE+9, 32'h0022_0033);
	WriteSREG(BASE+10, 32'h0044_0055);
	WriteSREG(BASE+11, 32'h0066_0077);
	WriteSREG(BASE+12, 32'h0088_0099);
	@(posedge clk);
	
	PutPacketInFIFO36(32'hA0B0C0D0,16);
	@(posedge clk);
	@(posedge clk);
	#10000;
	@(posedge clk);
	//PutPacketInFIFO36(32'hE0F0A0B0,36);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
     end

   initial #20000 $finish;
endmodule // prot_eng_tx_tb
