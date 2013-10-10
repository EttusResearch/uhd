`timescale 1ns/1ps

module cvita_insert_tlast_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("cvita_insert_tlast_tb.vcd");
   initial $dumpvars(0,cvita_insert_tlast_tb);

   task send_packet;
      input [63:0] data_start;
      input [31:0] len;
      
      begin
	 if(len < 9)
	   begin
	      {i_tlast, i_tdata} <= { 1'b1, data_start[63:48],len[15:0], data_start[31:0] };
	      i_tvalid <= 1;
	      @(posedge clk);
	      i_tvalid <= 0;
	   end
	 else
	   begin
	      {i_tlast, i_tdata} <= { 1'b0, data_start[63:48],len[15:0], data_start[31:0] };
	      i_tvalid <= 1;
	      @(posedge clk);
	      repeat(((len-2)/2)-1+len[0])
		begin
		   i_tdata <= i_tdata + 64'h0000_0002_0000_0002;
		   @(posedge clk);
		end
	      i_tdata <= i_tdata + 64'h0000_0002_0000_0002;
	      i_tlast <= 1;
	      @(posedge clk);
	      i_tvalid <= 1'b0;
	   end // else: !if(len < 3)
      end
   endtask // send_packet
   
   initial
     begin
	#1000 reset = 0;
	#200000;
	$finish;
     end
   
   reg [63:0] i_tdata;
   reg 	      i_tlast;
   reg 	      i_tvalid;
   wire       i_tready;

   wire [63:0] o_tdata;
   wire        o_tlast, o_tvalid, o_tready, o_tlast_regen;
   
   initial
     begin
	i_tvalid <= 0;
	
	while(reset)
	  @(posedge clk);
	@(posedge clk);

	send_packet(64'hA0000000_A0000001, 24);
	send_packet(64'hA0000000_A0000001, 20);
	send_packet(64'hA0000000_A0000001, 16);
	send_packet(64'hA0000000_A0000001, 12);
	send_packet(64'hA0000000_A0000001, 8);
	send_packet(64'hA0000000_A0000001, 4);
	send_packet(64'hA0000000_A0000001, 4);
	send_packet(64'hA0000000_A0000001, 8);
	send_packet(64'hA0000000_A0000001, 12);
     end // initial begin

   cvita_insert_tlast dut
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(o_tdata), .o_tlast(o_tlast_regen), .o_tvalid(o_tvalid), .o_tready(o_tready));

   assign o_tready = 1;
   

   always @(posedge clk)
     if(o_tvalid & o_tready)
       begin
	  $display ("TLAST %x\t TLAST_REGEN %x",i_tlast, o_tlast_regen);
	  if(i_tlast != o_tlast_regen)
	    $display("ERROR!!!!!!");
       end
endmodule // cvita_insert_tlast_tb
