`timescale 1ns/1ps

module new_rx_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("new_rx_tb.vcd");
   initial $dumpvars(0,new_rx_tb);

   initial
     begin
	#1000 reset = 0;
	#30000;
	$finish;
     end
   
   reg [7:0]   set_addr;
   reg [31:0]  set_data;
   reg 	       set_stb = 1'b0;
   
   reg [63:0]  vita_time;
   reg [31:0]  sample;
   reg 	       strobe;
   
   wire        run, full;

   wire [63:0] err_tdata;
   wire        err_tlast, err_tvalid, err_tready;

   wire [63:0] o_tdata;
   wire        o_tlast, o_tvalid;
   reg 	       o_tready;
   
   task send_command;
      input [63:0] send_time;
      input 	   send_at;
      input 	   chain;
      input 	   reload;
      input 	   stop;
      input [31:0] len;

      begin
	 set_stb <= 1;
	 set_addr <= 0;
	 set_data <= { send_at, chain, reload, stop, len };
	 @(posedge clk);
	 set_stb <= 1;
	 set_addr <= 1;
	 set_data <= send_time[63:32];
	 @(posedge clk);
	 set_stb <= 1;
	 set_addr <= 2;
	 set_data <= send_time[31:0];
	 @(posedge clk);
	 set_stb <= 0;
	 @(posedge clk);
      end
   endtask // send_command
   
   initial
     begin
	o_tready <= 0;
	while(reset)
	  @(posedge clk);
	set_stb <= 1;   // Set Max Length of Packet
	set_addr <= 8; 
	set_data <= 18;
	@(posedge clk);
	set_stb <= 1;   // Set SID
	set_addr <= 9; 
	set_data <= 32'hF00D_1234;
	@(posedge clk);
	
	send_command(64'h100/*time*/, 1/*send at*/, 0/*chain*/, 0/*reload*/,0/*stop*/,150/*len*/);
	send_command(64'h200/*time*/, 1/*send at*/, 0/*chain*/, 0/*reload*/,0/*stop*/,4/*len*/);
	//send_command(64'h100/*time*/, 1/*send at*/, 0/*chain*/, 0/*reload*/,0/*stop*/,5/*len*/);

	#8000;
	o_tready <= 1;
     end // initial begin
   
   always @(posedge clk)
     if(reset)
       vita_time <= 0;
     else
       vita_time <= vita_time + 1;

   new_rx_control #(.BASE(0)) rx_control
     (.clk(clk), .reset(reset), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time),
      .run(run), .eob(eob), .strobe(strobe), .full(full),
      .err_tdata(err_tdata), .err_tlast(err_tlast), .err_tvalid(err_tvalid), .err_tready(err_tready),
      .debug());

   new_rx_framer #(.BASE(8)) rx_framer
     (.clk(clk), .reset(reset), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time),
      .strobe(strobe), .sample(sample), .run(run), .eob(eob), .full(full),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
      );
   
   always @*
     strobe <= run;

   /*
   always @(posedge clk)
     if(reset)
       sample <= 0;
     else if(run)
       sample <= sample + 1;
    */
   always @* sample <= vita_time[31:0];
   
   always @(posedge clk)
     if(o_tvalid & o_tready)
       if(o_tlast)
	 $display("%x\tLAST\n",o_tdata);
       else
	 $display("%x",o_tdata);

   assign err_tready = 1;
   
   always @(posedge clk)
     if(err_tvalid & err_tready)
       if(err_tlast)
	 $display("\t\t\t\tERR LAST \t%x",err_tdata);
       else
	 $display("\t\t\t\tERR\t\t%x",err_tdata);
   
endmodule // new_rx_tb
