module fifo_extram_tb();
   
   reg clk 	 = 0;
   reg sram_clk  = 0;
   reg reset 	 = 1;
   reg clear 	 = 0;
   
   initial #1000 reset = 0;
   always #125 clk = ~clk;
   always #100 sram_clk = ~sram_clk;
   
   reg [15:0] f18_data = 0;
   reg f18_sof = 0, f18_eof = 0;
   
   wire [17:0] f18_in = {f18_eof,f18_sof,f18_data};
   reg src_rdy_f18i  = 0;
   wire dst_rdy_f18i;

   wire [17:0] f18_out;
   wire src_rdy_f18o;
   reg dst_rdy_f18o  = 0;

   wire [17:0] f18_int;
   wire        src_rdy_f18int, dst_rdy_f18int;
       
   wire [17:0] sram_d;
   wire [18:0] sram_a;
   wire [1:0]  sram_bw;
   wire        sram_we, sram_adv, sram_ce, sram_oe, sram_mode, sram_zz;
   wire [15:0] f1_occ;
   
   fifo_short #(.WIDTH(18)) fifo_short
     (.clk(sram_clk), .reset(reset), .clear(clear),
      .datain(f18_in), .src_rdy_i(src_rdy_f18i), .dst_rdy_o(dst_rdy_f18i), .space(),
      .dataout(f18_int), .src_rdy_o(src_rdy_f18int), .dst_rdy_i(dst_rdy_f18int), .occupied(f1_occ[4:0]) );
   
   assign f1_occ[15:5] = 0;
   
   fifo_extram fifo_extram
     (.reset(reset), .clear(clear),
      .datain(f18_int), .src_rdy_i(src_rdy_f18int), .dst_rdy_o(dst_rdy_f18int), .space(), .occ_in(f1_occ),
      .dataout(f18_out), .src_rdy_o(src_rdy_f18o), .dst_rdy_i(dst_rdy_f18o), .occupied(), .space_in(7),
      .sram_clk(sram_clk), .sram_a(sram_a), .sram_d(sram_d), .sram_we(sram_we),
      .sram_bw(sram_bw), .sram_adv(sram_adv), .sram_ce(sram_ce), .sram_oe(sram_oe),
      .sram_mode(sram_mode), .sram_zz(sram_zz));

`define idt 1
`ifdef idt
   wire [15:0]  dummy16;
   wire [1:0] 	dummy2;
   
   idt71v65603s150 
     ram_model(.A(sram_a[17:0]),
	       .adv_ld_(sram_adv),                  // advance (high) / load (low)
               .bw1_(0), .bw2_(0), .bw3_(0), .bw4_(0),   // byte write enables (low)
               .ce1_(0), .ce2(1), .ce2_(0),          // chip enables
               .cen_(sram_ce),                     // clock enable (low)
	       .clk(sram_clk),                      // clock
	       .IO({dummy16,sram_d[15:0]}), 
	       .IOP({dummy2,sram_d[17:16]}),                  // data bus
               .lbo_(sram_mode),                     // linear burst order (low)
               .oe_(sram_oe),                      // output enable (low)
               .r_w_(sram_we));                    // read (high) / write (low)
`else
   cy1356 ram_model(.d(sram_d),.clk(sram_clk),.a(sram_a),
		    .bws(2'b00),.we_b(sram_we),.adv_lb(sram_adv),
		    .ce1b(0),.ce2(1),.ce3b(0),
		    .oeb(sram_oe),.cenb(sram_ce),.mode(sram_mode) );
`endif // !`ifdef idt
   
   always @(posedge sram_clk)
     if(dst_rdy_f18o & src_rdy_f18o)
       $display("Read: %h",f18_out);

   always @(posedge sram_clk)
     if(dst_rdy_f18int & src_rdy_f18int)
       $display("Write: %h",f18_int);

   initial $dumpfile("fifo_extram_tb.vcd");
   initial $dumpvars(0,fifo_extram_tb);

   task SendPkt;
      input [15:0] data_start;
      input [31:0] data_len;
      begin
	 @(posedge sram_clk);
	 f18_data      = data_start;
	 f18_sof       = 1;
	 f18_eof       = 0;
	 src_rdy_f18i  = 1;
	 while(~dst_rdy_f18i)
	   #1;
	 @(posedge sram_clk);
	 repeat(data_len - 2)
	   begin
	      f18_data 	= f18_data + 16'h0101;
	      f18_sof 	= 0;
	      while(~dst_rdy_f18i)
		@(posedge sram_clk);

	      @(posedge sram_clk);
	   end
	 f18_data 	= f18_data + 16'h0101;
	 f18_eof 	= 1;
	 while(~dst_rdy_f18i)
	   #1;
	 @(posedge sram_clk);
	 src_rdy_f18i = 0;
	 f18_data  = 0;
	 f18_eof   = 0;
      end
   endtask // SendPkt
   
   initial
     begin
	@(negedge reset);
	@(posedge sram_clk);
	@(posedge sram_clk);
	#10000;
	@(posedge sram_clk);
	SendPkt(16'hA0B0, 100);
	#10000;
	//SendPkt(16'hC0D0, 220);
     end

   initial
     begin
	#20000;
	dst_rdy_f18o  = 1;
     end
   
   initial #200000 $finish;
endmodule // fifo_extram_tb

