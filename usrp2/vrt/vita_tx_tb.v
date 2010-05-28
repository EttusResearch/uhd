

module vita_tx_tb;

   localparam DECIM  = 8'd4;
   localparam INTERP = 8'd4;
   
   localparam MAXCHAN=1;
   localparam NUMCHAN=1;
   
   reg clk 	     = 0;
   reg reset 	     = 1;

   initial #1000 reset = 0;
   always #50 clk = ~clk;

   initial $dumpfile("vita_tx_tb.vcd");
   initial $dumpvars(0,vita_tx_tb);

   wire [(MAXCHAN*32)-1:0] sample, sample_tx;
   wire        strobe, run;
   reg [35:0] data_o  = 36'h0;
   reg 	      src_rdy = 0;
   wire       dst_rdy;
   
   wire [63:0] vita_time;

   reg 	       set_stb = 0;
   reg [7:0]   set_addr;
   reg [31:0]  set_data;
   wire        set_stb_dsp;
   wire [7:0]  set_addr_dsp;
   wire [31:0] set_data_dsp;

   wire        sample_dst_rdy, sample_src_rdy;
   wire [64+4+(MAXCHAN*32)-1:0] sample_data_o, sample_data_tx;

   time_64bit #(.TICKS_PER_SEC(100000000), .BASE(0)) time_64bit
     (.clk(clk), .rst(reset),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .pps(0), .vita_time(vita_time));

   wire [35:0] 			data_tx;
   wire 			src_rdy_tx, dst_rdy_tx;
   wire 			sample_dst_rdy_tx, sample_src_rdy_tx;
   
   fifo_long #(.WIDTH(36)) fifo_short
     (.clk(clk), .reset(reset), .clear(0),
      .datain(data_o), .src_rdy_i(src_rdy), .dst_rdy_o(dst_rdy),
      .dataout(data_tx), .src_rdy_o(src_rdy_tx), .dst_rdy_i(dst_rdy_tx));
   
   vita_tx_deframer #(.BASE(16), .MAXCHAN(MAXCHAN)) vita_tx_deframer
     (.clk(clk), .reset(reset), .clear(0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .data_i(data_tx), .dst_rdy_o(dst_rdy_tx), .src_rdy_i(src_rdy_tx),
      .sample_fifo_o(sample_data_tx), 
      .sample_fifo_dst_rdy_i(sample_dst_rdy_tx), .sample_fifo_src_rdy_o(sample_src_rdy_tx),
      .fifo_occupied(), .fifo_full(), .fifo_empty() );

   vita_tx_control #(.BASE(16), .WIDTH(MAXCHAN*32)) vita_tx_control
     (.clk(clk), .reset(reset), .clear(0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time), .underrun(underrun),
      .sample_fifo_i(sample_data_tx), 
      .sample_fifo_dst_rdy_o(sample_dst_rdy_tx), .sample_fifo_src_rdy_i(sample_src_rdy_tx),
      .sample(sample_tx), .run(run_tx), .strobe(strobe_tx));
   
   tx_dsp_model tx_dsp_model
     (.clk(clk), .reset(reset), .run(run_tx), .interp(INTERP), .strobe(strobe_tx), .sample(sample_tx[31:0] ));

   task write_setting;
      input [7:0] addr;
      input [31:0] data;
      begin
	 set_stb <= 0;
	 @(posedge clk);
	 set_addr <= addr;
	 set_data <= data;
	 set_stb  <= 1;
	 @(posedge clk);
	 set_stb <= 0;
      end
   endtask // write_setting
   
   initial 
     begin
	@(negedge reset);
	@(posedge clk);
	write_setting(4,32'h14900008);  // VITA header
	write_setting(5,32'hF00D1234);  // VITA streamid
	write_setting(6,32'h98765432);  // VITA trailer
	write_setting(7,8);  // Samples per VITA packet
	write_setting(8,NUMCHAN);  // Samples per VITA packet
	#10000;
	queue_vita_packets(32'h300, 106, 32'hF00D_1234, 32'h55AA_AA55);
	//queue_vita_packets(32'h300, 6, 32'hF00D_1234, 32'h0);
	queue_vita_packets(32'h600, 9, 32'h9876_ABCD, 32'h0);
	
	#300000 $finish;
     end

   task queue_vita_packets;
      input [31:0] sendtime;
      input [15:0] samples;
      input [15:0] word;
      input [31:0] trailer;
      
      reg [15:0]   i;
      
      begin
	 @(posedge clk);
	 src_rdy <= 1;
	 data_o <= {4'b0001,4'h1,1'b0,|trailer,2'h3,8'hF0,(16'd5+samples+|trailer)}; // header
	 @(posedge clk);
	 data_o <= {4'b0000,32'h0}; // streamid
	 @(posedge clk);
	 data_o <= {4'b0000,32'h0}; // SECS
	 @(posedge clk);
	 data_o <= {4'b0000,32'h0}; // TICS
	 @(posedge clk);
	 data_o <= {4'b0000,sendtime}; // TICS
	 @(posedge clk);

	 for(i=0;i<samples-1;i=i+1)
	   begin
	      data_o <= {4'b0000,i,word}; // Payload
	      @(posedge clk);
	   end
	 if(trailer==0)
	   begin
 	      data_o <= {4'b0010,i,16'hBEEF}; // Last Payload
	      @(posedge clk);
	   end
	 else
	   begin
 	      data_o <= {4'b0000,i,16'hBEEF}; // Last Payload
	      @(posedge clk);
 	      data_o <= {4'b0010,trailer}; // Last Payload
	      @(posedge clk);
	   end
	 src_rdy <= 0;
	 @(posedge clk);
	 
      end
   endtask // queue_vita_packets
   
endmodule // vita_tx_tb


module tx_dsp_model
  (input clk, input reset,
   input run,
   input [7:0] interp,
   output strobe,
   input [31:0] sample);

   cic_strober strober(.clock(clk), .reset(reset), .enable(run), .rate(interp), .strobe_fast(1), .strobe_slow(strobe));

   always @(posedge clk)
     if(strobe)
       $display("Time %d, Sent Sample %x",$time,sample);
   
   
endmodule // tx_dsp_model
