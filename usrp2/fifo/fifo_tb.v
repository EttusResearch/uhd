module fifo_tb();
   
   reg clk = 0;
   reg rst = 1;
   reg clear = 0;
   initial #1000 rst = 0;
   always #50 clk = ~clk;
   
   reg [31:0] f36_data = 0;
   reg [1:0] f36_occ = 0;
   reg f36_sof = 0, f36_eof = 0;
   reg f36_src_rdy;
   wire f36_dst_rdy;
   
   wire [7:0] ll_data;
   wire ll_src_rdy, ll_dst_rdy, ll_sof, ll_eof;
   wire [35:0] err_dat;
   wire        err_src_rdy, err_dst_rdy;

   fifo36_to_ll8 fifo36_to_ll8
     (.clk(clk),.reset(rst),.clear(clear),
      .f36_data({f36_occ,f36_eof,f36_sof,f36_data}),.f36_src_rdy_i(f36_src_rdy),.f36_dst_rdy_o(f36_dst_rdy),
      .ll_data(ll_data),.ll_sof(ll_sof),.ll_eof(ll_eof),
      .ll_src_rdy(ll_src_rdy),.ll_dst_rdy(ll_dst_rdy));

   assign ll_dst_rdy = 1;
   
   always @(posedge clk)
     if(ll_src_rdy)
       $display("LL: SOF %d, EOF %d, DAT %x",ll_sof,ll_eof,ll_data);
   
   initial $dumpfile("fifo_tb.vcd");
   initial $dumpvars(0,fifo_tb);

   initial
     begin
	@(negedge rst);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	f36_src_rdy <= 1;
	{f36_occ,f36_eof,f36_sof,f36_data} <= { 2'b00,1'b0,1'b1,32'h00010203};
	@(posedge clk);
	{f36_occ,f36_eof,f36_sof,f36_data} <= { 2'b00,1'b0,1'b0,32'h04050607};
	@(posedge clk);
	{f36_occ,f36_eof,f36_sof,f36_data} <= { 2'b00,1'b0,1'b0,32'h08090a0b};
	@(posedge clk);
	{f36_occ,f36_eof,f36_sof,f36_data} <= { 2'b11,1'b1,1'b0,32'h0c0d0e0f};
	@(posedge clk);
	f36_src_rdy <= 0;
     end
   
   initial #4000 $finish;
endmodule // longfifo_tb
