//
// Copyright 2012 Ettus Research LLC
//

`timescale  1 ps / 1 ps

module eth_dispatch_tb();

 
   // Clocking and reset interface
   reg clk; 
   reg reset; 
   reg clear; 
   // Setting register interface
   reg set_stb; 
   reg [15:0] set_addr; 
   reg [31:0] set_data;
   // Input 68bit AXI-Stream interface (from MAC)
   wire [63:0] in_tdata; 
   wire [3:0]  in_tuser; 
   wire        in_tlast; 
   wire        in_tvalid; 
   wire        in_tready;
   // Output AXI-Stream interface to VITA Radio Core
   wire [63:0] vita_tdata;
   wire [3:0]  vita_tuser; 
   wire        vita_tlast; 
   wire        vita_tvalid; 
   wire        vita_tready;
   // Output AXI-Stream interface to ZPU
   wire [63:0] zpu_tdata; 
   wire [3:0]  zpu_tuser; 
   wire        zpu_tlast; 
   wire        zpu_tvalid; 
   wire        zpu_tready;
   // Output AXI-Stream interface to cross-over MAC
   wire [63:0] xo_tdata; 
   wire [3:0]  xo_tuser; 
   wire        xo_tlast; 
   wire        xo_tvalid; 
   wire        xo_tready;
   
   reg [63:0] data_in;
   reg [3:0]  user_in;
   reg 	      valid_in;
   wire       ready_in;
   reg 	      last_in;
   
   eth_dispatch
     #(.BASE(0))
   eth_dispatch_i
     (
      // Clocking and reset interface
      .clk(clk), 
      .reset(reset), 
      .clear(clear), 
      // Setting register interface
      .set_stb(set_stb), 
      .set_addr(set_addr), 
      .set_data(set_data),
      // Input 68bit AXI-Stream interface (from MAC)
      .in_tdata(in_tdata), 
      .in_tuser(in_tuser), 
      .in_tlast(in_tlast), 
      .in_tvalid(in_tvalid), 
      .in_tready(in_tready),
      // Output AXI-STream interface to VITA Radio Core
      .vita_tdata(vita_tdata),
      .vita_tuser(vita_tuser),
      .vita_tlast(vita_tlast), 
      .vita_tvalid(vita_tvalid), 
      .vita_tready(vita_tready),
      // Output AXI-Stream interface to ZPU
      .zpu_tdata(zpu_tdata), 
      .zpu_tuser(zpu_tuser), 
      .zpu_tlast(zpu_tlast), 
      .zpu_tvalid(zpu_tvalid), 
      .zpu_tready(zpu_tready),
      // Output AXI-Stream interface to cross-over MAC
      .xo_tdata(xo_tdata), 
      .xo_tuser(xo_tuser), 
      .xo_tlast(xo_tlast), 
      .xo_tvalid(xo_tvalid), 
      .xo_tready(xo_tready)
      );
    
   //
   // Define Clocks
   //
   initial begin
      clk = 1'b1;
   end
   
   // 125MHz clock
   always #4000 clk = ~clk;

   //
   // Good starting state
   //
     initial begin
	reset <= 0;
	clear <= 0;
	set_stb <= 0;
	set_addr <= 0;
	set_data <= 0;
	data_in <= 0;
	user_in <= 0;
	valid_in <= 0;
	last_in <= 0;
	
     end

   

   //
   // Task Libaray
   //
  task write_setting_bus;
      input [15:0] address;
      input [31:0] data;

     begin

	@(negedge clk);
	set_stb = 1'b0;
	set_addr = 16'h0;
	set_data = 32'h0;
	@(negedge clk);
	set_stb = 1'b1;
	set_addr = address;
	set_data = data;
	@(negedge clk);
	set_stb = 1'b0;
	set_addr = 16'h0;
	set_data = 32'h0;

      end
   endtask // write_setting_bus
 
   
   task  enqueue_line;
      input last;
      input [2:0] keep;
      input [63:0] data;
      begin
         data_in <= {keep, data};
	 last_in <= last;
         valid_in <= 1;
         while (~ready_in) begin
            @(negedge clk);
         end
         @(negedge clk);
         data_in <= 0;
	 last_in <= 0;	 
         valid_in <= 0;
      end
   endtask // enqueue_line

   task enqueue_arp_req;
      input [47:0] src_mac;
      input [31:0] src_ip;
      input [47:0] dst_mac;
      input [31:0] dst_ip;

      begin
	 @(negedge clk);
	 // Line 0
	 enqueue_line( 0, 3'b0, {48'h0,16'hffff});
	 // Line 1 - Eth
	 enqueue_line( 0, 3'b0, {32'hffffffff,src_mac[47:16]});
	 // Line 2 - Eth+ARP (HTYPE = 1, PTYPE = 0x0800)
	 enqueue_line( 0, 3'b0, {src_mac[15:0],16'h0806,16'h0001,16'h0800});
	 // Line 3 -  HLEN=6, PLEN=4 OPER=1
	 enqueue_line( 0, 3'b0, {8'h06,8'h04,16'h0001,src_mac[47:16]});
	 // Line 4 - ARP
	 enqueue_line( 0, 3'b0, {src_mac[15:0],src_ip[31:0],dst_mac[47:32]});
	 // Line 5 - ARP
	 enqueue_line( 1, 3'b0, {dst_mac[31:0],dst_ip[31:0]});
      end
   endtask // enqueue_arp_req
   

   
   reg [11:0] frame_count =12'h0;
    
   task  enqueue_vita_pkt;
      // We assume that we always have SID and TSF fields.
      input [47:0] mac;
      input [31:0] ip;
      input [15:0] udp;
      input [15:0] vita_size;
      input [63:0] vita_tsf;
      input [31:0] vita_sid;
     
      
      integer i;
      reg [15:0] j;
      reg [19:0] vrl_size;
      reg [15:0] udp_size;
      reg [15:0] ip_size;

      
      begin
	 vrl_size = vita_size + 3;
	 udp_size = vrl_size*4 + 8;
	 ip_size = udp_size + 20;
	 @(negedge clk);
	 // Line 0
	 enqueue_line( 0, 3'b0, {48'h0,mac[47:32]});
	 // Line 1 - Eth
	 enqueue_line( 0, 3'b0, {mac[31:0],32'h11223344});
	 // Line 2 - Eth+IP
	 enqueue_line( 0, 3'b0, {16'h5566,16'h0800,16'h0000,ip_size});
	 // Line 3 - IP
	 enqueue_line( 0, 3'b0, 'h11<<16);
	 // Line 4 - IP
	 enqueue_line( 0, 3'b0, {32'h09080706, ip});
	 // Line 5 - UDP
	 enqueue_line( 0, 3'b0, {16'h1234, udp, udp_size, 16'h0});
         // Line 6 - VRL
	 enqueue_line( 0, 3'b0, {"VRLP",frame_count,vrl_size});	 
	 // Line 7 - VRT
	 enqueue_line( 0, 3'b0, {16'b0001000000010000, vita_size,vita_sid}); //vita hdr + SID
	 enqueue_line( 0, 3'b0, vita_tsf);
	 j = 0;
	 
	 for (i = 6; i < vita_size; i = i + 2) begin
	    enqueue_line( 0 , 3'b0, {j,j+16'h1,j+16'h2,j+16'h3});
	    j = j + 4;	    
         end
	 
	 if (i-vita_size==0) // 2x32words to finish VITA packet.
	   enqueue_line( 1, 3'b0, {j,j+16'h1,j+16'h2,j+16'h3});
	 else // 1x32bit word to finish VITA packet
	   enqueue_line( 1, 3'h4, {j,j+16'h1,j+16'h2,j+16'h3});
      end	 
  
   endtask // enqueue_packet
   

   //
   // Simulation specific testbench is included here
   //
`include "simulation_script.v"


   //
   // Input FIFO
   //
   axi_fifo_short
     #(.WIDTH(69)) axi_fifo_short_in
       (
	.clk(clk), 
	.reset(reset), 
	.clear(clear),
	.o_tdata({in_tlast,in_tuser,in_tdata}),
	.o_tvalid(in_tvalid),
	.o_tready(in_tready),
	.i_tdata({last_in,user_in,data_in}),
	.i_tvalid(valid_in),
	.i_tready(ready_in),
	.space(),
	.occupied()
	);  

   //
   // Output Sinks
   //
   axi_probe_tb 
     #(.FILENAME("zpu.txt"),.VITA_PORT0(60000),.VITA_PORT1(60001)) axi_probe_tb_zpu 
       (
	.clk(clk),
	.reset(reset),
	.clear(clear),
	.tdata(zpu_tdata),
	.tvalid(zpu_tvalid),
	.tready(zpu_tready),
	.tlast(zpu_tlast)
	);

   assign zpu_tready = 1'b1;

   axi_probe_tb 
     #(.FILENAME("xo.txt"),.VITA_PORT0(60000),.VITA_PORT1(60001)) axi_probe_tb_xo 
       (
	.clk(clk),
	.reset(reset),
	.clear(clear),
	.tdata(xo_tdata),
	.tvalid(xo_tvalid),
	.tready(xo_tready),
	.tlast(xo_tlast)
	);

   assign xo_tready = 1'b1;
   		     
   axi_probe_tb 
     #(.FILENAME("vita.txt"),.VITA_PORT0(60000),.VITA_PORT1(60001),.START_AT_VRL(1)) axi_probe_tb_vita 
       (
	.clk(clk),
	.reset(reset),
	.clear(clear),
	.tdata(vita_tdata),
	.tvalid(vita_tvalid),
	.tready(vita_tready),
	.tlast(vita_tlast)
	);

   assign vita_tready = 1'b1;

endmodule // eth_dispatch_tb
