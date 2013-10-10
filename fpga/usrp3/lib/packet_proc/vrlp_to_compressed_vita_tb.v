`timescale 1ns/1ps

module vrlp_to_compressed_vita_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("vrlp_to_compressed_vita_tb.vcd");
   initial $dumpvars(0,vrlp_to_compressed_vita_tb);

   task send_packet;
      input [63:0] data_start;
      input [31:0] len;
      
      begin
	 // Send a packet
	 @(posedge clk);
	 {i_tlast, i_tdata} <= { 1'b0, data_start };
	 i_tvalid <= 1;
	 @(posedge clk);
	 i_tdata <= 64'hAAAA_BBBB_CCCC_0000;
	 
	 repeat(len-2)
	   begin
	      i_tvalid <= 1;
	      @(posedge clk);
	      i_tdata <= i_tdata + 1;
	   end
	 i_tlast <= 1;
	 i_tdata <= i_tdata + 1;
	 @(posedge clk);
	 i_tvalid <= 1'b0;
	 
	 @(posedge clk);
      end
   endtask // send_packet
   
   
   initial
     begin
	#1000 reset = 0;
	#200000;
	$finish;
     end
   
   wire [63:0]  o_tdata;
   reg [63:0] 	i_tdata;
   wire [2:0] 	o_tuser;
   reg [2:0] 	i_tuser;
   reg 		i_tlast;
   wire 	o_tlast;
   wire		o_tvalid, i_tready;
   reg 		i_tvalid, o_tready;
   reg 		i_terror;
   
   localparam RPT_COUNT = 16;
   
   initial
     begin
	i_tvalid <= 0;
	o_tready <= 0;
	
	while(reset)
	  @(posedge clk);
	@(posedge clk);

	//send_packet(64'hA0,3'd0, 16, 0);
	send_packet(64'hAABC_0008_DEAD_BEEF, 4);
	send_packet(64'h7DEF_0008_8765_4321, 4);
	send_packet(64'hAABC_0007_F00D_1234, 4);
	send_packet(64'h7DEF_0007_ABCD_4321, 4);
	o_tready <= 1;
	//send_packet(64'hC0,3'd0, 16, 1);
	//send_packet(64'hD0,3'd0, 16, 0);
	//send_packet(64'hE0,3'd0, 16, 0);
	//send_packet(64'hF0,3'd0, 16, 0);

	@(posedge clk);

     end // initial begin

   wire i_terror_int, i_tlast_int, i_tready_int, i_tvalid_int;
   wire [2:0] i_tuser_int;
   wire [63:0] i_tdata_int;
   wire o_tlast_int, o_tready_int, o_tvalid_int;
   wire [2:0] o_tuser_int;
   wire [63:0] o_tdata_int;
   wire [63:0] vrlp_tdata;
   wire        vrlp_tlast, vrlp_tvalid, vrlp_tready;
   
   axi_fifo #(.WIDTH(65), .SIZE(10)) fifo
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata({i_tlast_int,i_tdata_int}), .o_tvalid(i_tvalid_int), .o_tready(i_tready_int));

   compressed_vita_to_vrlp dut0
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(i_tdata_int), .i_tlast(i_tlast_int), .i_tvalid(i_tvalid_int), .i_tready(i_tready_int),
      .o_tdata(vrlp_tdata), .o_tlast(vrlp_tlast), .o_tvalid(vrlp_tvalid), .o_tready(vrlp_tready));

   vrlp_to_compressed_vita dut1
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(vrlp_tdata), .i_tlast(vrlp_tlast), .i_tvalid(vrlp_tvalid), .i_tready(vrlp_tready),
      .o_tdata(o_tdata_int), .o_tlast(o_tlast_int), .o_tvalid(o_tvalid_int), .o_tready(o_tready_int));

   axi_fifo #(.WIDTH(65), .SIZE(10)) fifo_out
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({o_tlast_int,o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
      .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready));

   always @(posedge clk)
     if(o_tvalid & o_tready)
       begin
	  $display("%x",o_tdata);
	  if(o_tlast)
	    $display("======EOF========");
       end
endmodule // vrlp_to_compressed_vita_tb
