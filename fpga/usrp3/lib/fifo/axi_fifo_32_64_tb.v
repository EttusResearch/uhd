`timescale 1ns/1ps

module axi_fifo_32_64_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("axi_fifo_32_64_tb.vcd");
   initial $dumpvars(0,axi_fifo_32_64_tb);

   task send_packet;
      input [63:0] data_start;
      input [2:0]  user;
      input [31:0] len;
      
      begin
	 @(posedge clk);
	 {i_tuser, i_tlast, i_tdata} <= { 3'd0, 1'b0, data_start };
	 repeat(len-1)
	   begin
	      i_tvalid <= 1;
	      @(posedge clk);
	      i_tdata <= i_tdata + 64'h0000_0002_0000_0002;
	   end
	 i_tuser <= user;
	 i_tlast <= 1;
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
   
   reg [63:0] i_tdata;
   reg [2:0]  i_tuser;
   reg 	      i_tlast;
   reg 	      i_tvalid;
   wire       i_tready;

   wire [63:0] i_tdata_int;
   wire [2:0]  i_tuser_int;
   wire        i_tlast_int, i_tvalid_int, i_tready_int;

   wire [63:0] o_tdata;
   wire [31:0] o_tdata_int, o_tdata_int2;
   wire [2:0]  o_tuser;
   wire [1:0]  o_tuser_int, o_tuser_int2;
   wire        o_tlast, o_tlast_int, o_tvalid, o_tvalid_int, o_tready, o_tready_int;
   wire        o_tlast_int2, o_tvalid_int2, o_tready_int2;
   
   localparam RPT_COUNT = 16;
   
   initial
     begin
	i_tvalid <= 0;
	
	while(reset)
	  @(posedge clk);
	@(posedge clk);

	send_packet(64'hA0000000_A0000001, 3'd7, 4);
	@(posedge clk);
     end // initial begin

   axi_fifo #(.WIDTH(68), .SIZE(10)) fifo
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({i_tlast,i_tuser,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata({i_tlast_int,i_tuser_int,i_tdata_int}), .o_tvalid(i_tvalid_int), .o_tready(i_tready_int));

   axi_fifo64_to_fifo32 dut
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(i_tdata_int), .i_tuser(i_tuser_int), .i_tlast(i_tlast_int), .i_tvalid(i_tvalid_int), .i_tready(i_tready_int),
      .o_tdata(o_tdata_int), .o_tuser(o_tuser_int), .o_tlast(o_tlast_int), .o_tvalid(o_tvalid_int), .o_tready(o_tready_int));

   /*
   axi_fifo #(.WIDTH(35), .SIZE(10)) fifo_middle
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({o_tlast_int,o_tuser_int,o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
      .o_tdata({o_tlast_int2,o_tuser_int2,o_tdata_int2}), .o_tvalid(o_tvalid_int2), .o_tready(o_tready_int2));
*/
   assign o_tdata_int2 = o_tdata_int;
   assign o_tlast_int2 = o_tlast_int;
   assign o_tuser_int2 = o_tuser_int;
   assign o_tvalid_int2 = o_tvalid_int;
   assign o_tready_int = o_tready_int2;
   
   axi_fifo32_to_fifo64 dut2
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(o_tdata_int2), .i_tuser(o_tuser_int2), .i_tlast(o_tlast_int2), .i_tvalid(o_tvalid_int2), .i_tready(o_tready_int2),
      .o_tdata(o_tdata), .o_tuser(o_tuser), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

   assign o_tready = 1'b1;
   
   always @(posedge clk)
     if(i_tvalid & i_tready)
       $display("IN: TUSER %x\tTLAST %x\tTDATA %x", i_tuser, i_tlast, i_tdata);

   always @(posedge clk)
     if(o_tvalid_int & o_tready_int)
       $display("\t\t\t\t\t\tMIDDLE: TUSER %x\tTLAST %x\tTDATA %x", o_tuser_int, o_tlast_int, o_tdata_int);

   always @(posedge clk)
     if(o_tvalid & o_tready)
       $display("\t\t\t\t\t\t\t\t\t\t\tOUT: TUSER %x\tTLAST %x\tTDATA %x", o_tuser, o_tlast, o_tdata);

endmodule // axi_fifo_32_64_tb
