`timescale 1ns/1ps

module ll8_to_axi64_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("ll8_to_axi64_tb.vcd");
   initial $dumpvars(0,ll8_to_axi64_tb);

   initial
     begin
	#1000 reset = 0;
	#2000000;
	$finish;
     end
   
   wire [63:0]  tdata, tdata_int;
   wire [3:0] 	tuser, tuser_int;
   wire 	tlast, tlast_int;
   wire		tvalid, tvalid_int, tready, tready_int;

   reg [7:0] 	ll_data;
   reg 		ll_eof, ll_error, ll_src_rdy;
   wire 	ll_dst_rdy;

   wire [7:0] 	ll_data2;
   wire 	ll_eof2, ll_src_rdy2, ll_dst_rdy2;
   
   localparam RPT_COUNT = 12;
   
   initial
     begin
	ll_src_rdy <= 0;
 
	while(reset)
	  @(posedge clk);

	@(posedge clk);
	
	{ll_error, ll_eof, ll_data} <= { 1'b0, 1'b0, 8'hA0 };
	repeat(RPT_COUNT-1)
	  begin
	     ll_src_rdy <= 1;
	     @(posedge clk);
	     ll_data <= ll_data + 1;
	  end
	ll_eof <= 1;
	ll_data <= ll_data + 1;
	@(posedge clk);

	{ll_error, ll_eof, ll_data} <= { 1'b0, 1'b0, 8'hC0 };
	repeat(RPT_COUNT-1)
	  begin
	     ll_src_rdy <= 1;
	     @(posedge clk);
	     ll_data <= ll_data + 1;
	  end
	ll_eof <= 1; ll_error <= 1;
	ll_data <= ll_data + 1;
	@(posedge clk);
	ll_src_rdy <= 1'b0;

     end

   ll8_to_axi64 #(.START_BYTE(6), .LABEL(8'h89)) ll8_to_axi64
     (.clk(clk), .reset(reset), .clear(1'b0),
      .ll_data(ll_data), .ll_eof(ll_eof), .ll_error(ll_error), .ll_src_rdy(ll_src_rdy), .ll_dst_rdy(ll_dst_rdy),
      .axi64_tdata(tdata), .axi64_tlast(tlast), .axi64_tuser(tuser), .axi64_tvalid(tvalid), .axi64_tready(tready) );
   
   axi_fifo_short #(.WIDTH(69)) axi_fifo_short
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({tlast,tuser,tdata}), .i_tvalid(tvalid), .i_tready(tready),
      .o_tdata({tlast_int,tuser_int,tdata_int}), .o_tvalid(tvalid_int), .o_tready(tready_int));

   axi64_to_ll8 #(.START_BYTE(6)) axi64_to_ll8
     (.clk(clk), .reset(reset), .clear(1'b0),
      .axi64_tdata(tdata_int), .axi64_tlast(tlast_int), .axi64_tuser(tuser_int), .axi64_tvalid(tvalid_int), .axi64_tready(tready_int),
      .ll_data(ll_data2), .ll_eof(ll_eof2), .ll_src_rdy(ll_src_rdy2), .ll_dst_rdy(ll_dst_rdy2) );

   /*
   always @(posedge clk)
     if(ll_src_rdy2 & ll_dst_rdy2)
       $display("EOF %x\tDATA %x",ll_eof2, ll_data2);

   */
   assign ll_dst_rdy2 = 1;
   
   always @(posedge clk)
     if(tvalid_int & tready_int)
       $display("TERR %x\tTUSER %x\tTLAST %x\tTDATA %x",tuser_int[3],tuser_int[2:0], tlast_int, tdata_int);
   
endmodule // ll8_to_axi64_tb
