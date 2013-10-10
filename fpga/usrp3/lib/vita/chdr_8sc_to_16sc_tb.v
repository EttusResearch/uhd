//Purpose: to test 8 to 16 converter
`timescale 1ns/1ps

module chdr_8sc_to_16sc_tb();
   
   reg clk = 0;
   reg reset = 1;
   //generate clock
   always #10 clk = ~clk; 

   initial $dumpfile("chdr_8sc_to_16sc_tb.vcd");
   initial $dumpvars(0,chdr_8sc_to_16sc_tb);
   
   //tells when to finish
   initial
     begin
   	#50 reset = 0;
   	#50000; 
   	$finish;
     end

   //setting registers and wire
   reg [63:0] i_tdata;
   reg        i_tlast = 0;
   reg        i_tvalid = 0;
   wire       i_tready ;
   reg [7:0]  set_addr;
   reg [31:0] set_data;
   reg 	      set_stb;
   

   wire [63:0] o_tdata;
   wire        o_tlast;
   
   wire	       o_tvalid;
   reg 	       o_tready;

   chdr_8sc_to_16sc #(.BASE(89))dut
     (.clk(clk), .reset(reset),
      .set_data(set_data), .set_stb(set_stb), .set_addr(set_addr),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready), .debug());
   
   //if you want to feed a bigger input change array sizes here

   reg [63:0]  data[0:7];
   initial $readmemh("from8_to_x.hex", data);

   //test packet loop
   task test_packet;
      input [15:0] len;
      input [31:0] sid;
      reg [1:0]    index;
      
      begin
	 index <= 0;
	 @(posedge clk) ;
	 //send header 
   	 i_tdata <= {1'b0, 1'b0, 1'b1, 1'b0, 12'h0, (len + 16'd16),sid};
	 i_tvalid <= 1;
	 i_tlast <= 0;
	 @(posedge clk);
	 while (i_tready != 1)
	   @(posedge clk);
	 i_tdata <= {64'b0};
   	 @(posedge clk);
	 while (i_tready != 1)
	   @(posedge clk);
	 //-1 for last bit accounting
	 repeat ( len[15:3] + (len[2]|len[1]|len[0]) - 1 )
	   begin
	      i_tdata <= {data[index]}; 
   	      index <= index+1;
	      @(posedge clk);
	      while (i_tready != 1)
		@(posedge clk);
	   end
   	 i_tlast <= 1'b1;
	 i_tdata <= {data[index]}; 
	 @(posedge clk);
	 while (i_tready != 1)
	   @(posedge clk);
	 i_tvalid <= 0;
      end
   endtask // test_packet

   //test_destination loop

   task test_destination;
      input enable;
      input [15:0] dest_home;
      
      
      begin
         @(posedge clk);
         
         set_data <= {enable,dest_home};
         set_addr <= 89;
         set_stb <= 1;
         
         @(posedge clk);
         set_stb <= 0;
      end
   endtask



   
   
   //main loop	
   initial
     begin
	i_tvalid <= 0;
	o_tready <= 1;
	
	i_tdata <= 0;
	
	@(negedge reset);
	@(posedge clk);
	@(posedge clk);

	/* Uncomment to test without changed sid
	 test_destination(0,16'hFEED);
	 test_packet(20, 32'hDEAD_BEEF);
	 */
	test_destination(1,16'hFEED);
	test_packet(2, 32'hDEAD_BEEF);
	#1000;
	test_packet(4, 32'hDEAD_BEEF);
	#1000;
	test_packet(6, 32'hDEAD_BEEF);
	#1000;
	test_packet(8, 32'hDEAD_BEEF);
	#1000;
	test_packet(10, 32'hDEAD_BEEF);
	#1000;
	test_packet(12, 32'hDEAD_BEEF);
	#1000;
	test_packet(14, 32'hDEAD_BEEF);
	#1000;
	test_packet(16, 32'hDEAD_BEEF);
	#1000;
	
     end



endmodule
