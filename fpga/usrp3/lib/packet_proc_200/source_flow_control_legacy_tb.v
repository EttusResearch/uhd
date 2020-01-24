//
// Copyright 2016 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`timescale 1ns/1ps

module source_flow_control_legacy_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("source_flow_control_legacy_tb.vcd");
   initial $dumpvars(0,source_flow_control_legacy_tb);

   initial
     begin
	#1000 reset = 0;
	#20000;
	$finish;
     end
   
   reg [63:0]  tdata;
   wire [63:0] tdata_int;
   reg 	       tlast;
   wire        tlast_int;
   reg 	       tvalid = 1'b0;
   wire        tvalid_int;
   wire        tready, tready_int;
   
   reg [63:0]  fc_tdata;
   reg 	       fc_tlast, fc_tvalid;
   wire        fc_tready;

   wire [63:0] out_tdata;
   wire        out_tlast, out_tready, out_tvalid;

   wire [15:0] occ_in, occ_out;
   reg 	       set_stb = 0;
   reg [7:0]   set_addr;
   reg [31:0]  set_data;
   
   
   task send_fc_packet;
      input [31:0] seqnum;
      input [31:0] sid;
      input always_go;
      
      begin
	 @(posedge clk);
	 fc_tlast <= 1'b0;
	 fc_tdata <= { 1'b1, 1'b0, 1'b0, 1'b0, 12'hABC, 16'd4, sid };
	 fc_tvalid <= 1;
	 @(posedge clk);
	 fc_tlast <= 1'b1;
	 //fc_tdata <= { 52'h0,seqnum };
	 fc_tdata <= { 31'h0,always_go, seqnum };
	 @(posedge clk);
	 fc_tvalid <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet

   task send_packet;
      input ec;
      input timed;
      input [11:0] seqnum;
      input [31:0] sid;
      input [63:0] vtime;
      input [15:0] addr;
      input [31:0] data;
      
      begin
	 // Send a packet
	 @(posedge clk);
	 tlast <= 1'b0;
	 tdata <= { ec, 1'b0, timed, 1'b0, seqnum, timed ? 16'd6 : 16'd4, sid };
	 tvalid <= 1;
	 @(posedge clk);
	 if(timed)
	   begin
	      tdata <= vtime;
	      @(posedge clk);
	   end
	 tlast <= 1'b1;
	 tdata <= { 16'h0, addr, data };
	 @(posedge clk);
	 tlast <= 1'b0;
	 tvalid <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet
   
   initial
     begin
	tvalid <= 1'b0;
	while(reset)
	  @(posedge clk);
	@(posedge clk);
	// Set flow control window to be 2
	set_stb <= 1;
	set_addr <= 0;
	set_data <= 2;
	@(posedge clk);
	set_stb <= 0;
	// ExtContext. Time. Seq=0, SID=DEAD_6789, Time=10
	send_packet(1'b1,1'b1,12'h0,32'hDEAD_6789,64'h10,16'hB,32'hF00D_1234);
	send_packet(1'b1,1'b1,12'h1,32'hDEAD_6789,64'h20,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h2,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h3,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h4,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h5,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h6,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h7,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h8,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	#500;
	// Consumed 2 packets
	send_fc_packet(32'd1,32'h3,1'b0);
	#300;
	// Consumed 1 packet
	send_fc_packet(32'd2,32'h3,1'b0);
	#500;	
	// Consumed 2 packets
	send_fc_packet(32'd4,32'h3,1'b0);
	#400;	
	// Send same SEQ ID again to test it causes no changes.
	send_fc_packet(32'd4,32'h3,1'b0);
	#300;
	// Consumed 1 packet
	send_fc_packet(32'd5,32'h3,1'b0);
	#500;	
	// Consumed 2 packets
	send_fc_packet(32'd7,32'h3,1'b0);
	#500;	
	send_packet(1'b1,1'b1,12'h9,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'hA,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	#300;
	// Consumed 1 packet
	send_fc_packet(32'd8,32'h3,1'b0);
	//
	// Now force internal sequence count to close to wrap value to test corner case
	//
	#100;
	source_flow_control_legacy.current_seqnum <= 32'hFFFF_FFFC;
	#100;
	send_fc_packet(32'hFFFF_FFFA,32'h3,1'b0);
	#100;
	send_packet(1'b1,1'b1,12'hFFC,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	#200;
	send_packet(1'b1,1'b1,12'hFFD,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'hFFE,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'hFFF,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h000,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h001,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h002,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	#200;
	// Consumed 2 packets
	send_fc_packet(32'hFFFF_FFFC,32'h3,1'b0);
	#200;
	// Consumed 2 packets
	send_fc_packet(32'hFFFF_FFFE,32'h3,1'b0);
	send_packet(1'b1,1'b1,12'h003,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h004,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	#200;
	// Consumed 2 packets
	send_fc_packet(32'h0,32'h3,1'b0);
	#200;
	// Consumed 2 packets
	send_fc_packet(32'h2,32'h3,1'b0);
	#500;
	//
	// Again force internal sequence count to close to wrap value to test new corner case
	//
	#100;
	source_flow_control_legacy.current_seqnum <= 32'hFFFF_FFFC;
	#100;
	send_fc_packet(32'hFFFF_FFFA,32'h3,1'b0);
	#100;
	send_packet(1'b1,1'b1,12'hFFC,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	#200;
	send_packet(1'b1,1'b1,12'hFFD,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'hFFE,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'hFFF,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h000,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h001,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h002,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	#200;
	// Consumed 1 packets
	send_fc_packet(32'hFFFF_FFFB,32'h3,1'b0);
	#200;
	// Consumed 1 packets
	send_fc_packet(32'hFFFF_FFFC,32'h3,1'b0);
	send_packet(1'b1,1'b1,12'h003,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h004,32'hDEAD_6789,64'h40,16'hC,32'hABCD_4321);
	#200;
	// Consumed 1 packets
	send_fc_packet(32'hFFFF_FFFD,32'h3,1'b0);
	#200;
	// Consumed 1 packets
	send_fc_packet(32'hFFFF_FFFE,32'h3,1'b0);
	#200;
	// Consumed 1 packets
	send_fc_packet(32'hFFFF_FFFF,32'h3,1'b0);
	#200;	
	// Consumed 1 packets
	send_fc_packet(32'h0,32'h3,1'b0);
	#200;	
	// Consumed 1 packets
	send_fc_packet(32'h1,32'h3,1'b0);
	#200;	
	// Consumed 1 packets
	send_fc_packet(32'h2,32'h3,1'b0);
	#500;
	


	
     end

   axi_fifo #(.WIDTH(65), .SIZE(10)) fifo_in
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({tlast,tdata}), .i_tvalid(tvalid), .i_tready(tready),
      .o_tdata({tlast_int,tdata_int}), .o_tvalid(tvalid_int), .o_tready(tready_int),
      .occupied(occ_in));

   source_flow_control_legacy source_flow_control_legacy
     (.clk(clk), .reset(reset), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .fc_tdata(fc_tdata), .fc_tlast(fc_tlast), .fc_tvalid(fc_tvalid), .fc_tready(fc_tready),
      .in_tdata(tdata_int), .in_tlast(tlast_int), .in_tvalid(tvalid_int), .in_tready(tready_int),
      .out_tdata(out_tdata), .out_tlast(out_tlast), .out_tvalid(out_tvalid), .out_tready(out_tready)
      );

   wire [63:0] dump_tdata;
   wire        dump_tlast, dump_tvalid, dump_tready;
   
   axi_fifo #(.WIDTH(65), .SIZE(10)) fifo_out
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({out_tlast,out_tdata}), .i_tvalid(out_tvalid), .i_tready(out_tready),
      .o_tdata({dump_tlast,dump_tdata}), .o_tvalid(dump_tvalid), .o_tready(dump_tready),
      .occupied(occ_out));

   assign dump_tready = 0;
   
   always @(posedge clk)
     if(out_tvalid & out_tready)
       begin
	  $display("%x",out_tdata);
	  if(out_tlast)
	    $display("TLAST");
       end
endmodule // source_flow_control_legacy_tb
