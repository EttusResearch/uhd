`timescale 1ns/1ps

//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module window_tb();
   xlnx_glbl glbl (.GSR(),.GTS());

   localparam STR_SINK_FIFOSIZE = 11;
  
   reg clk, reset;
 
   localparam PORTS = 5;

   wire [63:0] noci_tdata[PORTS-1:0];
   wire [PORTS-1:0]       noci_tlast;
   wire [PORTS-1:0]       noci_tvalid;
   wire [PORTS-1:0]       noci_tready;

   wire [63:0] noco_tdata[PORTS-1:0];
   wire [PORTS-1:0]       noco_tlast;
   wire [PORTS-1:0]       noco_tvalid;
   wire [PORTS-1:0]       noco_tready;
   
   wire [63:0] src_tdata;
   wire	       src_tlast, src_tvalid;
   wire        src_tready;

   reg [63:0]  cmdout_tdata;
   reg 	       cmdout_tlast, cmdout_tvalid;
   wire        cmdout_tready;

   wire [63:0] dst_tdata;
   wire        dst_tlast, dst_tvalid;
   wire        dst_tready = 1;
 	       
   reg 	       set_stb_xbar;
   reg [15:0]  set_addr_xbar;
   reg [31:0]  set_data_xbar;

   always
     #100 clk = ~clk;

   initial clk = 0;
   initial reset = 1;
   initial #1000 reset = 0;
   
   initial $dumpfile("window_tb.vcd");
   initial $dumpvars(0,window_tb);

   initial #3000000 $finish;


   axi_crossbar #(.FIFO_WIDTH(64), .DST_WIDTH(16), .NUM_INPUTS(PORTS), .NUM_OUTPUTS(PORTS)) crossbar
     (.clk(clk), .reset(reset), .clear(1'b0),
      .local_addr(8'd0),
      .pkt_present({noci_tvalid[4],noci_tvalid[3],noci_tvalid[2],noci_tvalid[1],noci_tvalid[0]}),
      
      .i_tdata({noci_tdata[4],noci_tdata[3],noci_tdata[2],noci_tdata[1],noci_tdata[0]}),
      .i_tlast({noci_tlast[4],noci_tlast[3],noci_tlast[2],noci_tlast[1],noci_tlast[0]}),
      .i_tvalid({noci_tvalid[4],noci_tvalid[3],noci_tvalid[2],noci_tvalid[1],noci_tvalid[0]}),
      .i_tready({noci_tready[4],noci_tready[3],noci_tready[2],noci_tready[1],noci_tready[0]}),

      .o_tdata({noco_tdata[4],noco_tdata[3],noco_tdata[2],noco_tdata[1],noco_tdata[0]}),
      .o_tlast({noco_tlast[4],noco_tlast[3],noco_tlast[2],noco_tlast[1],noco_tlast[0]}),
      .o_tvalid({noco_tvalid[4],noco_tvalid[3],noco_tvalid[2],noco_tvalid[1],noco_tvalid[0]}),
      .o_tready({noco_tready[4],noco_tready[3],noco_tready[2],noco_tready[1],noco_tready[0]}),

      .set_stb(set_stb_xbar), .set_addr(set_addr_xbar), .set_data(set_data_xbar),
      .rb_rd_stb(1'b0), .rb_addr(4'd0), .rb_data());
   
   // Generator on port 0
   wire        set_stb_0;
   wire [7:0]  set_addr_0;
   wire [31:0] set_data_0;
   
   noc_shell #(.STR_SINK_FIFOSIZE(STR_SINK_FIFOSIZE)) noc_shell_0
     (.bus_clk(clk), .bus_rst(reset),
      .i_tdata(noco_tdata[0]), .i_tlast(noco_tlast[0]), .i_tvalid(noco_tvalid[0]), .i_tready(noco_tready[0]),
      .o_tdata(noci_tdata[0]), .o_tlast(noci_tlast[0]), .o_tvalid(noci_tvalid[0]), .o_tready(noci_tready[0]),
      .clk(clk), .reset(reset),
      .set_data(set_data_0), .set_addr(set_addr_0), .set_stb(set_stb_0), .rb_data(64'd0),

      .cmdout_tdata(64'h0), .cmdout_tlast(1'b0), .cmdout_tvalid(1'b0), .cmdout_tready(),
      .ackin_tdata(), .ackin_tlast(), .ackin_tvalid(), .ackin_tready(1'b1),
      
      .str_sink_tdata(), .str_sink_tlast(), .str_sink_tvalid(), .str_sink_tready(1'b1), // unused port
      .str_src_tdata(src_tdata), .str_src_tlast(src_tlast), .str_src_tvalid(src_tvalid), .str_src_tready(src_tready)
      );

   file_source #(.BASE(8), .FILENAME("test.dat")) file_source
     (.clk(clk), .reset(reset),
      .set_data(set_data_0), .set_addr(set_addr_0), .set_stb(set_stb_0),
      .o_tdata(src_tdata), .o_tlast(src_tlast), .o_tvalid(src_tvalid), .o_tready(src_tready));
      
   // Simple FIR on port 1
   wire [31:0] set_data_1;
   wire [7:0]  set_addr_1;
   wire        set_stb_1;
   wire [63:0] s1o_tdata, s1i_tdata;
   wire        s1o_tlast, s1i_tlast, s1o_tvalid, s1i_tvalid, s1o_tready, s1i_tready;

   wire [31:0] pre_tdata, post_tdata;
   wire        pre_tlast, pre_tvalid, pre_tready;
   wire        post_tlast, post_tvalid, post_tready;

   wire [15:0] pre_i = pre_tdata[31:16];
   wire [15:0] pre_q = pre_tdata[15:0];
   wire [15:0] post_i = post_tdata[31:16];
   wire [15:0] post_q = post_tdata[15:0];
   
   noc_shell #(.STR_SINK_FIFOSIZE(STR_SINK_FIFOSIZE)) noc_shell_1
     (.bus_clk(clk), .bus_rst(reset),
      .i_tdata(noco_tdata[1]), .i_tlast(noco_tlast[1]), .i_tvalid(noco_tvalid[1]), .i_tready(noco_tready[1]),
      .o_tdata(noci_tdata[1]), .o_tlast(noci_tlast[1]), .o_tvalid(noci_tvalid[1]), .o_tready(noci_tready[1]),
      .clk(clk), .reset(reset),
      .set_data(set_data_1), .set_addr(set_addr_1), .set_stb(set_stb_1), .rb_data(64'd0),

      .cmdout_tdata(64'h0), .cmdout_tlast(1'b0), .cmdout_tvalid(1'b0), .cmdout_tready(),
      .ackin_tdata(), .ackin_tlast(), .ackin_tvalid(), .ackin_tready(1'b1),
      
      .str_sink_tdata(s1o_tdata), .str_sink_tlast(s1o_tlast), .str_sink_tvalid(s1o_tvalid), .str_sink_tready(s1o_tready),
      .str_src_tdata(s1i_tdata), .str_src_tlast(s1i_tlast), .str_src_tvalid(s1i_tvalid), .str_src_tready(s1i_tready)
      );

   wire [31:0] axis_config_tdata1;
   wire        axis_config_tvalid1, axis_config_tready1, axis_config_tlast1;
   
   axi_wrapper #(.BASE(8)) axi_wrapper_ce1
     (.clk(clk), .reset(reset),
      .set_stb(set_stb_1), .set_addr(set_addr_1), .set_data(set_data_1),
      .i_tdata(s1o_tdata), .i_tlast(s1o_tlast), .i_tvalid(s1o_tvalid), .i_tready(s1o_tready),
      .o_tdata(s1i_tdata), .o_tlast(s1i_tlast), .o_tvalid(s1i_tvalid), .o_tready(s1i_tready),
      .m_axis_data_tdata(pre_tdata),
      .m_axis_data_tlast(pre_tlast),
      .m_axis_data_tvalid(pre_tvalid),
      .m_axis_data_tready(pre_tready),
      .s_axis_data_tdata(post_tdata),
      .s_axis_data_tlast(post_tlast),
      .s_axis_data_tvalid(post_tvalid),
      .s_axis_data_tready(post_tready),
      .m_axis_config_tdata(axis_config_tdata1),
      .m_axis_config_tlast(axis_config_tlast1),
      .m_axis_config_tvalid(axis_config_tvalid1),
      .m_axis_config_tready(axis_config_tready1)
      );

   window #(.BASE(0)) window
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb_1), .set_addr(set_addr_1), .set_data(set_data_1),
      .i_tdata(pre_tdata), .i_tlast(pre_tlast), .i_tvalid(pre_tvalid), .i_tready(pre_tready),
      .o_tdata(post_tdata), .o_tlast(post_tlast), .o_tvalid(post_tvalid), .o_tready(post_tready));

   assign axis_config_tready1 = 1'b1;
   
   // Dumper on port 2
   noc_shell #(.STR_SINK_FIFOSIZE(STR_SINK_FIFOSIZE)) noc_shell_2
     (.bus_clk(clk), .bus_rst(reset),
      .i_tdata(noco_tdata[2]), .i_tlast(noco_tlast[2]), .i_tvalid(noco_tvalid[2]), .i_tready(noco_tready[2]),
      .o_tdata(noci_tdata[2]), .o_tlast(noci_tlast[2]), .o_tvalid(noci_tvalid[2]), .o_tready(noci_tready[2]),
      
      .clk(clk), .reset(reset),
      .set_data(), .set_addr(), .set_stb(), .rb_data(64'd0),

      .cmdout_tdata(64'h0), .cmdout_tlast(1'b0), .cmdout_tvalid(1'b0), .cmdout_tready(),
      .ackin_tdata(), .ackin_tlast(), .ackin_tvalid(), .ackin_tready(1'b1),
      
      .str_sink_tdata(dst_tdata), .str_sink_tlast(dst_tlast), .str_sink_tvalid(dst_tvalid), .str_sink_tready(dst_tready),
      .str_src_tdata(64'd0), .str_src_tlast(1'd0), .str_src_tvalid(1'b0), .str_src_tready() // unused port
      );

   // Control Source on port 3
   noc_shell #(.STR_SINK_FIFOSIZE(STR_SINK_FIFOSIZE)) noc_shell_3
     (.bus_clk(clk), .bus_rst(reset),
      .i_tdata(noco_tdata[3]), .i_tlast(noco_tlast[3]), .i_tvalid(noco_tvalid[3]), .i_tready(noco_tready[3]),
      .o_tdata(noci_tdata[3]), .o_tlast(noci_tlast[3]), .o_tvalid(noci_tvalid[3]), .o_tready(noci_tready[3]),
      
      .clk(clk), .reset(reset),
      .set_data(), .set_addr(), .set_stb(), .rb_data(64'd0),

      .cmdout_tdata(cmdout_tdata), .cmdout_tlast(cmdout_tlast), .cmdout_tvalid(cmdout_tvalid), .cmdout_tready(cmdout_tready),
      .ackin_tdata(), .ackin_tlast(), .ackin_tvalid(), .ackin_tready(1'b1),
      
      .str_sink_tdata(), .str_sink_tlast(), .str_sink_tvalid(), .str_sink_tready(1'b1), // unused port
      .str_src_tdata(64'd0), .str_src_tlast(1'd0), .str_src_tvalid(1'b0), .str_src_tready() // unused port
      );

   // ////////////////////////////////////////////////////////////////////////////////////
   
   task SetXbar;
      input [15:0] start_reg;
      input [7:0]  start_val;
      
      begin
	 repeat (PORTS)
	   begin
	      repeat (1)
		begin
		   SetXbar_reg(start_reg,start_val);
		   start_reg <= start_reg + 1;
		   @(posedge clk);
		end
	      start_val <= start_val + 1;
	      @(posedge clk);
	   end
      end
   endtask // SetXbar
   
   task SetXbar_reg;
      input [15:0] addr;
      input [31:0] data;
      begin
	 @(posedge clk);
	 set_stb_xbar <= 1'b1;
	 set_addr_xbar <= addr;
	 set_data_xbar <= data;
	 @(posedge clk);
	 set_stb_xbar <= 1'b0;
	 @(posedge clk);
      end
   endtask // set_xbar

   task SendCtrlPacket;
      input [11:0] seqnum;
      input [31:0] sid;
      input [63:0] data;
      
      begin
	 @(posedge clk);
	 cmdout_tdata <= { 4'h8, seqnum, 16'h16, sid };
	 cmdout_tlast <= 0;
	 cmdout_tvalid <= 1;
	 while(~cmdout_tready) #1;
	 
	 @(posedge clk);
	 cmdout_tdata <= data;
	 cmdout_tlast <= 1;
	 while(~cmdout_tready) #1;
	 
	 @(posedge clk);
	 cmdout_tvalid <= 0;
	 @(posedge clk);
      end
   endtask // SendCtrlPacket

   initial
     begin
	cmdout_tdata <= 64'd0;
	cmdout_tlast <= 1'b0;
	cmdout_tvalid <= 1'b0;
	
	@(negedge reset);
	@(posedge clk);
	SetXbar(256,0);
	
	@(posedge clk);
	// Port 0
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'h0, 32'h0000_0003}); // Command packet to set up source control window size
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'h1, 32'h0000_0001}); // Command packet to set up source control window enable
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'h3, 32'h8000_0001}); // Command packet to set up flow control
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'h8, 32'h0000_0001}); // Command packet to set up SID
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'hA, 32'h0000_0002}); // Command packet to set up Rate
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'hB, 32'h0000_0001}); // Command packet to set up send_time_field
	SendCtrlPacket(12'd0, 32'h0003_0000, {32'h9, 32'h0000_0200}); // Command packet to set up Len
	#10000;
	// Port 1
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h0, 32'h0000_0013}); // Command packet to set up source control window size
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h1, 32'h0000_0001}); // Command packet to set up source control window enable
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h3, 32'h8000_0001}); // Command packet to set up flow control
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h8, 32'h0001_0002}); // Rewrite SID, send on to port 2


	#10000;
	// Port 2
	SendCtrlPacket(12'd0, 32'h0003_0002, {32'h0, 32'h0000_0003}); // Command packet to set up source control window size
	SendCtrlPacket(12'd0, 32'h0003_0002, {32'h1, 32'h0000_0001}); // Command packet to set up source control window enable
	SendCtrlPacket(12'd0, 32'h0003_0002, {32'h3, 32'h8000_0001}); // Command packet to set up flow control

	#1000000;
	// WINDOW filter
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd0});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd1});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd2});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd3});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd4});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd5});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd6});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd7});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h11, 32'd8});  // frame_len  (FFTsize)

	#1000000;
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h10, 32'd100000});  // frame_len  (FFTsize)
	SendCtrlPacket(12'd0, 32'h0003_0001, {32'h11, 32'd100000});  // frame_len  (FFTsize)

     end

   reg in_packet = 0;

   integer outfile;
   
   initial
     begin
	outfile = $fopen("output.dat","wb");
	//src_tready <= 1'b1;
     end

   wire signed  [15:0] a,b,c,d;
   assign a = src_tdata[63:48];
   assign b = src_tdata[47:32];
   assign c = src_tdata[31:16];
   assign d = src_tdata[15:0];

   always @(posedge clk)
     if(src_tready & src_tvalid)
       begin
	  if(src_tlast)
	    in_packet <= 0;
	  else
	    in_packet <= 1;
	  if(in_packet)
	    begin
	       //$fwrite(outfile,"%u",{q_out[15:0],i_out[15:0]}); // Correct endianness for GR
	       //$write("%d,%d,%d,%d,",a,b,c,d);
	       $fwrite(outfile,"%u",{dst_tdata[47:32],dst_tdata[63:48]});
	       $fwrite(outfile,"%u",{dst_tdata[15:0],dst_tdata[31:16]});
	    end
       end

endmodule // window_tb
