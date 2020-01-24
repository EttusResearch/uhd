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

`timescale 1ns / 1ps
`define USRP2
//`define USRP2PLUS

`ifdef USRP2
 `define INT_WIDTH 36
 `define EXT_WIDTH 18
 `define RAM_DEPTH 19
 `define FIFO_DEPTH 8
 `define DUMP_VCD_FULL
 `define INT_CLK_PERIOD 5
 `define EXT_CLK_PERIOD 4
`elsif USRP2PLUS
 `define INT_WIDTH 36
 `define EXT_WIDTH 36
 `define RAM_DEPTH 18
 `define FIFO_DEPTH 8
 `define DUMP_VCD_FULL
 `define INT_CLK_PERIOD 5
 `define EXT_CLK_PERIOD 5
`endif //  `ifdef USRP2
   

module ext_fifo_tb();
  
   reg int_clk;
   reg ext_clk;
   reg rst;

   wire [`EXT_WIDTH-1:0] RAM_D_pi;
   wire [`EXT_WIDTH-1:0] RAM_D_po;
   wire [`EXT_WIDTH-1:0] RAM_D;
   wire 		RAM_D_poe;
   wire [`RAM_DEPTH-1:0] 	RAM_A;
   wire 		RAM_WEn;
   wire 		RAM_CENn;
   wire 		RAM_LDn;
   wire 		RAM_OEn;
   wire 		RAM_CE1n;
   reg [`INT_WIDTH-1:0] datain;
   reg 			src_rdy_i;                // WRITE
   wire 		dst_rdy_o;               // not FULL
   wire [`INT_WIDTH-1:0] dataout;
   reg [`INT_WIDTH-1:0]  ref_dataout;
   wire 		src_rdy_o;               // not EMPTY
   reg 			dst_rdy_i;
   integer 		ether_frame;
   
   // Clocks
   // Int clock is 100MHz
   // Ext clock is 125MHz
   initial 
     begin
	int_clk <= 0;
	ext_clk <= 0;
	ref_dataout <= 1;
	src_rdy_i <= 0;
	dst_rdy_i <= 0;
     end

   always
     #(`INT_CLK_PERIOD/2) int_clk <= ~int_clk;

   always
     #(`EXT_CLK_PERIOD/2) ext_clk <= ~ext_clk;

   initial
     begin
	datain <= 0;
	ether_frame <= 0;
	
	rst <= 1;
	repeat (5) @(negedge int_clk);
	rst <= 0;
	@(negedge int_clk);
	while (datain < 10000)
	  begin
	     @(negedge int_clk);
	     datain <= datain + dst_rdy_o;
	     src_rdy_i <= dst_rdy_o;
	     // Simulate inter-frame time	     
	     if (ether_frame == 1500)
	       begin
		  ether_frame <= 0;
		  repeat(1600)
		    begin
		       @(negedge int_clk);
		       src_rdy_i <= 0;
		    end
	       end
	     else
	       ether_frame <= ether_frame + dst_rdy_o;
	  end
     end // initial begin

   
   initial
     begin
	repeat (5) @(negedge int_clk);
	dst_rdy_i <= 1;

	while (src_rdy_o !== 1)
	  @(negedge int_clk);
	  
	// Fall through fifo, first output already valid
	if (dataout !== ref_dataout)
	  $display("Error: Expected %x, got %x @%d",ref_dataout, dataout, $time);
	ref_dataout <= ref_dataout + src_rdy_o ;
	
	// Decimate by 16 rate
	while (ref_dataout < 2000)
	  begin
	     @(negedge int_clk);
	     ref_dataout <= ref_dataout + src_rdy_o ;
	     dst_rdy_i <= src_rdy_o;
	     if ((dataout !== ref_dataout) && src_rdy_o)
	       $display("Error: Expected %x, got %x @%d",ref_dataout, dataout, $time);
	     @(negedge int_clk);
	     dst_rdy_i <= 0;
	     repeat(14) @(negedge int_clk);
	  end // while (ref_dataout < 10000)
	// Decimate by 8 rate
	while (ref_dataout < 4000)
	  begin
	     @(negedge int_clk);
	     ref_dataout <= ref_dataout + src_rdy_o ;
	     dst_rdy_i <= src_rdy_o;
	     if ((dataout !== ref_dataout) && src_rdy_o)
	       $display("Error: Expected %x, got %x @%d",ref_dataout, dataout, $time);
	     @(negedge int_clk);
	     dst_rdy_i <= 0;
	     repeat(6) @(negedge int_clk);
	  end // while (ref_dataout < 10000)
	// Decimate by 4 rate
	while (ref_dataout < 6000)
	  begin
	     @(negedge int_clk);
	     ref_dataout <= ref_dataout + src_rdy_o ;
	     dst_rdy_i <= src_rdy_o;
	     if ((dataout !== ref_dataout) && src_rdy_o)
	       $display("Error: Expected %x, got %x @%d",ref_dataout, dataout, $time);
	     @(negedge int_clk);
	     dst_rdy_i <= 0;
	     repeat(2) @(negedge int_clk);
	  end // while (ref_dataout < 10000)
	// Max rate
	while (ref_dataout < 10000)
	  begin
	     @(negedge int_clk);
	     ref_dataout <= ref_dataout + src_rdy_o ;
	     dst_rdy_i <= src_rdy_o;
	     if ((dataout !== ref_dataout) && src_rdy_o)
	       $display("Error: Expected %x, got %x @%d",ref_dataout, dataout, $time);
	     
	  end // while (ref_dataout < 10000)
	
	 @(negedge int_clk);
	$finish;
	
     end
   

/* -----\/----- EXCLUDED -----\/-----
 
   initial
     begin
	rst <= 1;
	repeat (5) @(negedge int_clk);
	rst <= 0;
	@(negedge int_clk);
	repeat (4000)
	  begin
	     @(negedge int_clk);
	     datain <= datain + dst_rdy_o;
	     src_rdy_i <= dst_rdy_o;
//	     @(negedge int_clk);
//	     src_rdy_i <= 0;
//	     @(negedge int_clk);
//	     dst_rdy_i <= src_rdy_o;
//	     @(negedge int_clk);
//	     dst_rdy_i <= 0;
//	     repeat (2) @(negedge int_clk);
	  end // repeat (1000)
	// Fall through fifo, first output already valid
	if (dataout !== ref_dataout)
	  $display("Error: Expected %x, got %x",ref_dataout, dataout);
	repeat (1000)
	  begin
	     @(negedge int_clk);
	     datain <= datain + dst_rdy_o ;
	     src_rdy_i <= dst_rdy_o;
	     @(negedge int_clk);
	     src_rdy_i <= 0;
	     @(negedge int_clk);
	     ref_dataout <= ref_dataout + src_rdy_o ;
	     dst_rdy_i <= src_rdy_o;
	     if ((dataout !== ref_dataout) && src_rdy_o)
	       $display("Error: Expected %x, got %x",ref_dataout, dataout);
	     @(negedge int_clk);
	     dst_rdy_i <= 0;
//	     repeat (2) @(negedge int_clk);
	  end // repeat (1000)
	repeat (1000)
	  begin
//	     @(negedge int_clk);
//	     datain <= datain + 1;
//	     src_rdy_i <= 1;
//	     @(negedge int_clk);
//	     src_rdy_i <= 0;
	     @(negedge int_clk);
	     ref_dataout <= ref_dataout + src_rdy_o;
	     dst_rdy_i <= src_rdy_o;
	     if ((dataout !== ref_dataout) && src_rdy_o)
	       $display("Error: Expected %x, got %x",ref_dataout, dataout);
	     @(negedge int_clk);
	     dst_rdy_i <= 0;
//	     repeat (2) @(negedge int_clk);
	  end // repeat (1000)
	
	$finish;
	
     end // initial begin


 -----/\----- EXCLUDED -----/\----- */
   ///////////////////////////////////////////////////////////////////////////////////
   // Simulation control                                                            //
   ///////////////////////////////////////////////////////////////////////////////////
   `ifdef DUMP_LX2_TOP
   // Set up output files
   initial begin
      $dumpfile("ext_fifo_tb.lx2");
      $dumpvars(1,ext_fifo_tb);
   end
   `endif

   `ifdef DUMP_LX2_FULL
   // Set up output files
   initial begin
      $dumpfile("ext_fifo_tb.lx2");
      $dumpvars(0,ext_fifo_tb);
   end
   `endif

   `ifdef DUMP_VCD_TOP
   // Set up output files
   initial begin
      $dumpfile("ext_fifo_tb.vcd");
      $dumpvars(1,ext_fifo_tb);
   end
   `endif

   `ifdef DUMP_VCD_TOP_PLUS_NEXT
   // Set up output files
   initial begin
      $dumpfile("ext_fifo_tb.vcd");
      $dumpvars(2,ext_fifo_tb);
   end
   `endif

   
   `ifdef DUMP_VCD_FULL
   // Set up output files
   initial begin
      $dumpfile("ext_fifo_tb.vcd");
      $dumpvars(0,ext_fifo_tb);
   end
   `endif

   // Update display every 10 us
   always #10000 $monitor("Time in uS ",$time/1000);

   wire [`EXT_WIDTH-1:0] RAM_D_pi_ext;
   wire [`EXT_WIDTH-1:0] RAM_D_po_ext;
   wire [`EXT_WIDTH-1:0] RAM_D_ext;
   wire 		 RAM_D_poe_ext;
   
   genvar      i;

   //
   // Instantiate IO for Bidirectional bus to SRAM
   //
   
   generate  
      for (i=0;i<`EXT_WIDTH;i=i+1)
        begin : gen_RAM_D_IO

	   IOBUF #(
		   .DRIVE(12),
		   .IOSTANDARD("LVCMOS25"),
		   .SLEW("FAST")
		   )
	     RAM_D_i (
		      .O(RAM_D_pi_ext[i]),
		      .I(RAM_D_po_ext[i]),
		      .IO(RAM_D[i]),
		      .T(RAM_D_poe_ext)
		      );
	end // block: gen_RAM_D_IO
      
   endgenerate

   wire [`RAM_DEPTH-1:0] 	RAM_A_ext;
   wire 		RAM_WEn_ext,RAM_LDn_ext,RAM_CE1n_ext,RAM_OEn_ext,RAM_CENn_ext;

   assign 		#1 RAM_D_pi = RAM_D_pi_ext;
   
   assign 		#1 RAM_D_po_ext = RAM_D_po;

   assign 		#1 RAM_D_poe_ext = RAM_D_poe;
   
   assign 		#2 RAM_WEn_ext = RAM_WEn;
   
   assign 		#2 RAM_LDn_ext = RAM_LDn;
   
   assign 		#2 RAM_CE1n_ext = RAM_CE1n;
   
   assign 		#2 RAM_OEn_ext = RAM_OEn;
   
   assign 		#2 RAM_CENn_ext = RAM_CENn;

   assign 		#2 RAM_A_ext = RAM_A;
   
   
   generate
      if (`EXT_WIDTH==18) begin: ram_tb_g1
	 idt71v65603s150  idt71v65603s150_i1
	   (
	    .A(RAM_A_ext[17:0]),
	    .adv_ld_(RAM_LDn_ext),                  // advance (high) / load (low)
	    .bw1_(1'b0), 
	    .bw2_(1'b0),
	    .bw3_(1'b1), 
	    .bw4_(1'b1),   // byte write enables (low)
	    .ce1_(RAM_CE1n_ext), 
	    .ce2(1'b1), 
	    .ce2_(1'b0),          // chip enables
	    .cen_(RAM_CENn_ext),                     // clock enable (low)
	    .clk(ext_clk),                      // clock
	    .IO({RAM_D[16:9],RAM_D[7:0]}), 
	    .IOP({RAM_D[17],RAM_D[8]}),                  // data bus
	    .lbo_(1'b0),                     // linear burst order (low)
	    .oe_(RAM_OEn_ext),                      // output enable (low)
	    .r_w_(RAM_WEn_ext)
	    );                    // read (high) / write (low)
      end // block: ram_tb_g1
      else if (`EXT_WIDTH==36) begin: ram_tb_g1
	 idt71v65603s150  idt71v65603s150_i1
	   (
	    .A(RAM_A_ext[17:0]),
	    .adv_ld_(RAM_LDn_ext),                  // advance (high) / load (low)
	    .bw1_(1'b0), 
	    .bw2_(1'b0),
	    .bw3_(1'b0), 
	    .bw4_(1'b0),   // byte write enables (low)
	    .ce1_(RAM_CE1n_ext), 
	    .ce2(1'b1), 
	    .ce2_(1'b0),          // chip enables
	    .cen_(RAM_CENn_ext),                     // clock enable (low)
	    .clk(ext_clk),                      // clock
	    .IO(RAM_D[31:0]), 
	    .IOP(RAM_D[35:32]),                  // data bus
	    .lbo_(1'b0),                     // linear burst order (low)
	    .oe_(RAM_OEn_ext),                      // output enable (low)
	    .r_w_(RAM_WEn_ext)
	    );                    // read (high) / write (low)
      end // block: ram_tb_g1
 
   endgenerate
   
/* -----\/----- EXCLUDED -----\/-----

	 
   cy1356 cy1356_i1
     ( .d(RAM_D),
       .clk(ext_clk), 
       .a(RAM_A_ext), 
       .bws(2'b00), 
       .we_b(RAM_WEn_ext), 
       .adv_lb(RAM_LDn_ext), 
       .ce1b(RAM_CE1n_ext), 
       .ce2(1'b1), 
       .ce3b(1'b0), 
       .oeb(RAM_OEn_ext), 
       .cenb(RAM_CENn_ext),
       .mode(1'b0)
       );
 -----/\----- EXCLUDED -----/\----- */

   
   ext_fifo
     #(.INT_WIDTH(`INT_WIDTH),.EXT_WIDTH(`EXT_WIDTH),.RAM_DEPTH(`RAM_DEPTH),.FIFO_DEPTH(`FIFO_DEPTH))
       ext_fifo_i1
	 (
	  .int_clk(int_clk),
	  .ext_clk(ext_clk),
	  .rst(rst),
	  .RAM_D_pi(RAM_D_pi),
	  .RAM_D_po(RAM_D_po),
	  .RAM_D_poe(RAM_D_poe),
	  .RAM_A(RAM_A),
	  .RAM_WEn(RAM_WEn),
	  .RAM_CENn(RAM_CENn),
	  .RAM_LDn(RAM_LDn),
	  .RAM_OEn(RAM_OEn),
	  .RAM_CE1n(RAM_CE1n),
	  .datain(datain),
	  .src_rdy_i(src_rdy_i),                // WRITE
	  .dst_rdy_o(dst_rdy_o),               // not FULL
	  .dataout(dataout),
	  .src_rdy_o(src_rdy_o),               // not EMPTY
	  .dst_rdy_i(dst_rdy_i)
	  );

endmodule // ext_fifo_tb
