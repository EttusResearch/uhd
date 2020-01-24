//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module axi_wrapper_tb();
   
   xlnx_glbl glbl (.GSR(),.GTS());
   
   localparam STR_SINK_FIFOSIZE = 9;
      
   reg clk, reset;
   always
     #100 clk = ~clk;

   initial clk = 0;
   initial reset = 1;
   initial #1000 reset = 0;
   
   initial $dumpfile("axi_wrapper_tb.vcd");
   initial $dumpvars(0,axi_wrapper_tb);

   initial #1000000 $finish;

   wire [31:0] set_data;
   wire [7:0]  set_addr;
   wire        set_stb;

   wire [63:0] noci_tdata[PORTS-1:0];
   wire        noci_tlast[PORTS-1:0];
   wire        noci_tvalid[PORTS-1:0];
   wire        noci_tready[PORTS-1:0];

   wire [63:0] noco_tdata[PORTS-1:0];
   wire        noco_tlast[PORTS-1:0];
   wire        noco_tvalid[PORTS-1:0];
   wire        noco_tready[PORTS-1:0];

   reg [63:0]  src_tdata;
   reg 	       src_tlast, src_tvalid;
   wire        src_tready;

   localparam PORTS = 4;

   wire [63:0] s1o_tdata, s1i_tdata;
   wire        s1o_tlast, s1i_tlast, s1o_tvalid, s1i_tvalid, s1o_tready, s1i_tready;
   
   wire [31:0] pre_tdata, post_tdata;
   wire        pre_tlast, post_tlast, pre_tvalid, post_tvalid, pre_tready, post_tready;
   wire [127:0] pre_tuser, post_tuser;
      
   axi_wrapper #(.BASE(8), .NUM_AXI_CONFIG_BUS(1), .CONFIG_BUS_FIFO_DEPTH(5), .SIMPLE_MODE(1)) axi_wrapper_ce1
     (.clk(clk), .reset(reset),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .i_tdata(src_tdata), .i_tlast(src_tlast), .i_tvalid(src_tvalid), .i_tready(src_tready),
      .o_tdata(s1i_tdata), .o_tlast(s1i_tlast), .o_tvalid(s1i_tvalid), .o_tready(s1i_tready),
      .m_axis_data_tdata(pre_tdata),
      .m_axis_data_tuser(pre_tuser),
      .m_axis_data_tlast(pre_tlast),
      .m_axis_data_tvalid(pre_tvalid),
      .m_axis_data_tready(pre_tready),
      .s_axis_data_tdata(post_tdata),
      .s_axis_data_tuser(post_tuser),
      .s_axis_data_tlast(post_tlast),
      .s_axis_data_tvalid(post_tvalid),
      .s_axis_data_tready(post_tready)
      );

   axi_fifo #(.WIDTH(33)) afifo
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({pre_tlast,pre_tdata}), .i_tvalid(pre_tvalid), .i_tready(pre_tready),
      .o_tdata({post_tlast,post_tdata}), .o_tvalid(post_tvalid), .o_tready(post_tready));

   assign s1i_tready = 1'b1;
   

   task SendPacket;
      input [3:0]  flags;
      input [11:0] seqnum;
      input [15:0] len;
      input [31:0] sid;
      input [63:0] data;
      
      begin
	 @(posedge clk);
	 src_tdata <= { flags, seqnum, len+16'd8 + (flags[1] ? 16'd8 : 16'd0), sid };
	 src_tlast <= 0;
	 src_tvalid <= 1;
	 @(posedge clk);
	 while(~src_tready)
	   @(posedge clk);

	 // send time if flags request it
	 if(flags[1])
	   begin
	      src_tdata <= 64'h0123_4567_89ab_cdef;
	      src_tlast <= 0;
	      src_tvalid <= 1;
	      @(posedge clk);
	      while(~src_tready)
		@(posedge clk);
	   end

	 src_tdata <= data;
	 repeat(len[15:3] + (len[2]|len[1]|len[0])- 1 )
	   begin
	      @(posedge clk);
	      while(~src_tready)
		@(posedge clk);
	      src_tdata <= src_tdata + 64'd1;
	   end
	 src_tlast <= 1;
	 @(posedge clk);
	 while(~src_tready)
	   @(posedge clk);
	 src_tvalid <= 0;
	 @(posedge clk);
      end
   endtask // SendPacket
   
   initial
     begin
	src_tdata <= 64'd0;
	src_tlast <= 1'b0;
	src_tvalid <= 1'b0;
	@(negedge reset);
	@(posedge clk);
	
	@(posedge clk);

	#10000;
	SendPacket(4'h0, 12'd7, 16'd64, 32'h0002_0003, 64'hAAAA_AAAA_0000_0000); // data packet
	SendPacket(4'h0, 12'd8, 16'd68, 32'h0004_0005, 64'hBBBB_BBBB_0000_0000); // data packet
	//SendPacket(4'h0, 12'd2, 16'd8, 32'h0000_0001, 64'hCCCC_CCCC_0000_0000); // data packet
	//SendPacket(4'h0, 12'd3, 16'd8, 32'h0000_0001, 64'hDDDD_DDDD_0000_0000); // data packet
	//SendPacket(4'h0, 12'd4, 16'd8, 32'h0000_0001, 64'hEEEE_EEEE_0000_0000); // data packet
	//SendPacket(4'h0, 12'd5, 16'd8, 32'h0000_0001, 64'hFFFF_FFFF_0000_0000); // data packet
	//SendPacket(4'h0, 12'd6, 16'd8, 32'h0000_0001, 64'h2222_2222_0000_0000); // data packet
     end

endmodule // axi_wrapper_tb
