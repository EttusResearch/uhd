//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`timescale 1ns/1ps

module radio_ctrl_proc_tb();

   reg clk    = 0;
   reg reset  = 1;

   always #10 clk = ~clk;

   initial $dumpfile("radio_ctrl_proc_tb.vcd");
   initial $dumpvars(0,radio_ctrl_proc_tb);

   initial
     begin
	#1000 reset = 0;
	#20000;
	$finish;
     end
   
   reg [63:0] vita_time = 64'd0;
   always @(posedge clk)
     if(reset) vita_time <= 64'd0;
     else vita_time <= vita_time + 64'd1;

   reg [63:0]  tdata;
   wire [63:0] tdata_int;
   reg 	       tlast;
   wire        tlast_int;
   reg 	       tvalid = 1'b0;
   wire        tvalid_int;
   wire        tready, tready_int;
   
   wire [7:0]  set_addr;
   wire [31:0] set_data;
   wire        set_stb;
   wire        ready = 1'b1;

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
	 tvalid <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet
   
   initial
     begin
	tvalid <= 1'b0;
	while(reset)
	  @(posedge clk);
	send_packet(1'b1,1'b0,12'h5,32'hDEAD_BEEF,64'h0,16'hB,32'hF00D_1234);
	send_packet(1'b1,1'b1,12'h6,32'hDEAD_6789,64'h20,16'hC,32'hABCD_4321);
	send_packet(1'b1,1'b1,12'h7,32'hDEAD_6789,64'h30,16'hC,32'hABCD_4321);
	//send_packet(.ec(1), .timed(0), .seqnum(5), .sid(32'hDEAD_BEEF), .vtime(0), .addr(16'hB), .data(32'hF00D_1234));
     end

   axi_fifo_short #(.WIDTH(65)) axi_fifo_short
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({tlast,tdata}), .i_tvalid(tvalid), .i_tready(tready),
      .o_tdata({tlast_int,tdata_int}), .o_tvalid(tvalid_int), .o_tready(tready_int));

   wire [63:0] resp_tdata;
   wire        resp_tlast, resp_tvalid, resp_tready;
   
   radio_ctrl_proc radio_ctrl_proc
     (.clk(clk), .reset(reset), .clear(1'b0),
      .ctrl_tdata(tdata_int), .ctrl_tlast(tlast_int), .ctrl_tvalid(tvalid_int), .ctrl_tready(tready_int),
      .resp_tdata(resp_tdata), .resp_tlast(resp_tlast), .resp_tvalid(resp_tvalid), .resp_tready(resp_tready),
      .vita_time(vita_time), .ready(ready),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .debug()
      );

   assign resp_tready = 1'b1;
   
   always @(posedge clk)
     if(resp_tvalid & resp_tready)
       begin
	  $display("%x",resp_tdata);
	  if(resp_tlast)
	    $display("TLAST");
       end
endmodule // radio_ctrl_proc_tb
