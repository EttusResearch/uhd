
module gpif_tb();
   
   reg sys_clk = 0;
   reg sys_rst = 1;
   reg gpif_clk = 0;
   reg gpif_rst = 1;
      
   reg [15:0] gpif_data;
   reg 	      WR = 0, EP = 0;

   wire       CF, DF;
   
   wire       gpif_full_d, gpif_full_c;
   wire [18:0] data_o, ctrl_o;
   wire        src_rdy, dst_rdy;
   wire        ctrl_src_rdy, ctrl_dst_rdy;

   assign ctrl_dst_rdy = 1;
   assign dst_rdy = 1;
   
   initial $dumpfile("gpif_tb.vcd");
   initial $dumpvars(0,gpif_tb);

   initial #1000 gpif_rst = 0;
   initial #1000 sys_rst = 0;
   always #64 gpif_clk <= ~gpif_clk;
   always #47.9 sys_clk <= ~sys_clk;

   wire [18:0] data_int;
   wire        src_rdy_int, dst_rdy_int;
   
   gpif_wr gpif_write
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst), 
      .gpif_data(gpif_data), .gpif_wr(WR), .gpif_ep(EP),
      .gpif_full_d(DF), .gpif_full_c(CF),
      
      .sys_clk(sys_clk), .sys_rst(sys_rst),
      .data_o(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int),
      .ctrl_o(ctrl_o), .ctrl_src_rdy_o(ctrl_src_rdy), .ctrl_dst_rdy_i(ctrl_dst_rdy) );

   packet_reframer tx_packet_reframer 
     (.clk(sys_clk), .reset(sys_rst), .clear(0),
      .data_i(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int),
      .data_o(data_o), .src_rdy_o(src_rdy), .dst_rdy_i(dst_rdy));

   packet_splitter #(.FRAME_LEN(256)) rx_packet_splitter
     (.clk(sys_clk), .reset(sys_rst), .clear(0),
      .data_i(data_o), .src_rdy_i(src_rdy), .dst_rdy_o(dst_rdy),
      .data_o(data_o), .src_rdy_o(src_rdy), .dst_rdy_i(dst_rdy));

   always @(posedge sys_clk)
     if(ctrl_src_rdy & ctrl_dst_rdy)
       $display("CTRL: %x",ctrl_o);

   always @(posedge sys_clk)
     if(src_rdy & dst_rdy)
       begin
	  if(data_o[16])
	    $display("<-------- DATA SOF--------->");
	  $display("DATA: %x",data_o);
	  if(data_o[17])
	    $display("<-------- DATA EOF--------->");
       end
   
   initial
     begin
	#10000;
	repeat (1)
	  begin
	     WR <= 1;
	     gpif_data <= 10;  // Length
	     @(posedge gpif_clk);
	     gpif_data <= 16'h00;
	     @(posedge gpif_clk);
	     repeat(254)
	       begin
		  gpif_data <= gpif_data + 1;
		  @(posedge gpif_clk);
	       end
	     WR <= 0;
	     repeat (20)
	       @(posedge gpif_clk);
	     WR <= 1;
	     gpif_data <= 16'h5;
	     @(posedge gpif_clk);
	     repeat(254)
	       begin
		  gpif_data <= gpif_data - 1;
		  @(posedge gpif_clk);
	       end
	  end
     end // initial begin
   
   initial #100000 $finish;
   
     
endmodule // gpif_tb
