`timescale 1ns/1ns

module fifo_extram36_tb();
   
   reg clk 	 = 0;
   reg sram_clk  = 0;
   reg rst 	 = 1;
   reg clear 	 = 0;

   reg Verbose = 0;   // 
   integer ErrorCount = 0;
   
   initial #1000 rst = 0;
//   always #125 clk = ~clk;
   task task_CLK;
      reg [7:0] ran;
      begin
	 while (1) begin
	    ran = $random;
	    if (ran[1])
	      #62 clk = ~clk;
	    else
	      #63 clk = !clk;
	 end
      end
   endtask // task_CLK
   initial task_CLK;
   
//   always #100 sram_clk = ~sram_clk;
   task task_SSRAM_clk;
      reg [7:0] ran;
      begin
	 while (1) begin
	    ran = $random;
	    if (ran[0])
	      #49 sram_clk = ~sram_clk;
	    else
	      #51 sram_clk = ~sram_clk;
	 end
      end
   endtask // task_SSRAM_clk
   initial task_SSRAM_clk;

   reg [31:0] f36_data = 32'hX;
   reg [1:0]  f36_occ = 0;
   reg 	      f36_sof = 0, f36_eof = 0;
   
   wire [35:0] f36_in = {1'b0,f36_occ,f36_eof,f36_sof,f36_data};
   reg 	       src_rdy_f36i  = 0;
   wire        dst_rdy_f36i;

   wire [35:0] f36_out;
   wire        src_rdy_f36o;
   reg 	       dst_rdy_f36o  = 0;

   wire [17:0] sram_d;
   wire [18:0] sram_a;
   wire [1:0]  sram_bw;
   wire        sram_we, sram_adv, sram_ce, sram_oe, sram_mode, sram_zz;

   reg [31:0]  ScoreBoard [524288:0];
   reg [18:0] put_index = 0;
   reg [18:0] get_index = 0;
   
//   integer     put_index = 0;
//   integer     get_index = 0;
   
   wire [15:0] DUT_space, DUT_occupied;

   fifo_extram36 fifo_extram36
     (.clk(clk), .reset(rst), .clear(clear),
      .datain(f36_in), .src_rdy_i(src_rdy_f36i), .dst_rdy_o(dst_rdy_f36i), .space(DUT_space),
      .dataout(f36_out), .src_rdy_o(src_rdy_f36o), .dst_rdy_i(dst_rdy_f36o), .occupied(DUT_occupied),
      .sram_clk(sram_clk), .sram_a(sram_a), .sram_d(sram_d), .sram_we(sram_we),
      .sram_bw(sram_bw), .sram_adv(sram_adv), .sram_ce(sram_ce), .sram_oe(sram_oe),
      .sram_mode(sram_mode), .sram_zz(sram_zz));

`define idt 1
`ifdef idt
   wire [15:0] dummy16;
   wire [1:0]  dummy2;
   
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
   cy1356 ram_model(.d(sram_d),.clk(~sram_clk),.a(sram_a),
		    .bws(2'b00),.we_b(sram_we),.adv_lb(sram_adv),
		    .ce1b(0),.ce2(1),.ce3b(0),
		    .oeb(sram_oe),.cenb(sram_ce),.mode(sram_mode) );
`endif

   task task_SSRAMMonitor;
      reg last_mode;
      reg last_clock;
      reg last_load;
      reg [18:0] sram_addr;

      begin
	 last_mode = 1'bX;
	 last_clock = 1'bX;
	 last_load = 1'bX;

	 @ (posedge Verbose);
	 $dumpvars(0,fifo_extram36_tb);
	 
	 $display("%t:%m\t*** Task Started",$time);
	 while (1) @ (posedge sram_clk) begin
	    if (sram_mode !== last_mode) begin
	       $display("%t:%m\tSSRAM mode:         %b",$time,sram_mode);
	       last_mode = sram_mode;
	    end
	    if (sram_adv !== last_load) begin
	       $display("%t:%m\tSSRAM adv/load:     %b",$time,sram_adv);
	       last_load = sram_adv;
	    end
	    if (sram_ce !== last_clock) begin
	       $display("%t:%m\tSSRAM clock enable: %b",$time,sram_ce);
	       last_clock = sram_ce;
	    end
	    if (sram_ce == 1'b0) begin
	       if (sram_adv == 1'b0) begin
//		  $display("%t:%m\tSSRAM Address Load A=%h",$time,sram_a);
		  sram_addr = sram_a;
	       end else begin
		  sram_addr = sram_addr + 1;
	       end
	       if (sram_oe == 1'b0) begin
		  $display("%t:%m\tSSRAM Read  Cycle A=%h(%h), D=%o",$time,sram_addr-2,sram_a,sram_d);
	       end
	       if (sram_we == 1'b0) begin
		  $display("%t:%m\tSSRAM Write Cycle A=%h(%h), D=%o",$time,sram_addr-2,sram_a,sram_d);
	       end
	       if ((sram_we == 1'b0) && (sram_oe == 1'b0)) begin
		 $display("%t:%m\t*** ERROR: _oe and _we both active",$time);
	       end
	       
	    end // if (sram_ce == 1'b0)
	    
	 end // always @ (posedge sram_clk)
      end
   endtask // task_SSRAMMonitor
   
   task ReadFromFIFO36;
      begin
	 $display("%t: Read from FIFO36",$time);
	 #1 dst_rdy_f36o <= 1;
	 while(1)
	   begin
	      while(~src_rdy_f36o)
		@(posedge clk);
	      $display("%t: Read: %h>",$time,f36_out);
	      @(posedge clk);
	   end
      end
   endtask // ReadFromFIFO36

   initial dst_rdy_f36o = 0;

   task task_ReadFIFO36;
      reg [7:0] ran;
      begin
      $display("%t:%m\t*** Task Started",$time);
      while (1) begin
	 //  Read on one of four clocks
	 #5 dst_rdy_f36o <= 1;
	 @(posedge clk);
	 if (src_rdy_f36o) begin
	    if (f36_out[31:0] != ScoreBoard[get_index]) begin
	       $display("%t:%m\tFIFO Get Error: R:%h, E:%h (%h)",$time,f36_out[31:0],ScoreBoard[get_index],get_index);
	       ErrorCount = ErrorCount + 1;
	    end else begin
	       if (Verbose)
		 $display("%t:%m\t(%5h) %o>",$time,get_index,f36_out);
	    end
	    get_index = get_index+1;
	 end else begin
	    if (ErrorCount >= 192)
	      $finish;
	 end // else: !if(src_rdy_f36o)
	 
	 #10;
	 ran = $random;
	 if (ran[2:0] != 3'b000) begin
	    dst_rdy_f36o <= 0;
	    if (ran[2] != 1'b0) begin
	       @(posedge clk);
	       @(posedge clk);
	       @(posedge clk);
	    end
	    if (ran[1] != 1'b0) begin
	       @(posedge clk);
	       @(posedge clk);
	    end
	    if (ran[0] != 1'b0) begin
	       @(posedge clk);
	    end
	 end
      end // while (1)
   end
      
   endtask // task_ReadFIFO36
   

   reg [15:0] count;
   
   task PutPacketInFIFO36;
      input [31:0] data_start;
      input [31:0] data_len;

      begin
	 count 	      = 4;
	 src_rdy_f36i = 1;
	 f36_data     = data_start;
	 f36_sof      = 1;
	 f36_eof      = 0;
	 f36_occ      = 0;
	
	 $display("%t: Put Packet in FIFO36",$time);
	 while(~dst_rdy_f36i)
	   #1; //@(posedge clk);
	 @(posedge clk);

	 $display("%t: <%h PPI_FIFO36: Entered First Line",$time,f36_data);
	 f36_sof <= 0;
	 while(count+4 < data_len)
	   begin
	      f36_data = f36_data + 32'h01010101;
	      count    = count + 4;
	      while(~dst_rdy_f36i)
		#1; //@(posedge clk);
	      @(posedge clk);
	      $display("%t: <%h PPI_FIFO36: Entered New Line",$time,f36_data);
	   end
	 f36_data  <= f36_data + 32'h01010101;
	 f36_eof   <= 1;
	 if(count + 4 == data_len)
	   f36_occ <= 0;
	 else if(count + 3 == data_len)
	   f36_occ <= 3;
	 else if(count + 2 == data_len)
	   f36_occ <= 2;
	 else
	   f36_occ <= 1;
	 while(~dst_rdy_f36i)
	   @(posedge clk);
	 @(posedge clk);
	 f36_occ      <= 0;
	 f36_eof      <= 0;
	 f36_data     <= 0;
	 src_rdy_f36i <= 0;
	 $display("%t: <%h PPI_FIFO36: Entered Last Line",$time,f36_data);
      end
   endtask // PutPacketInFIFO36

   task task_WriteFIFO36;
      integer i;
      reg [7:0] ran;
      
      begin
	 f36_data = 32'bX;
	 if (rst != 1'b0)
	   @ (negedge rst);
	 $display("%t:%m\t*** Task Started",$time);
	 #10;
	 src_rdy_f36i = 1;
	 f36_data = $random;
	 for (i=0; i<64; i=i+0 ) begin
	    @ (posedge clk) ;
	    if (dst_rdy_f36i) begin
	       if (Verbose)
		 $display("%t:%m\t(%5h) %o<",$time,put_index,f36_in);
	       ScoreBoard[put_index] = f36_in[31:0];
	       put_index = put_index + 1;
	       #5;
	       f36_data = $random;
	       i = i + 1;
	    end
	    ran = $random;
	    if (ran[1:0] != 2'b00) begin
	       @ (negedge clk);
	       src_rdy_f36i = 0;
	       #5;
	       @ (negedge clk) ;
	       src_rdy_f36i = 1;
	    end
	 end
	 src_rdy_f36i = 0;
	 f36_data = 32'bX;
//*	 if (put_index > 19'h3ff00)
//*	   Verbose = 1'b1;
	 
      end
   endtask // task_WriteFIFO36
   
   initial $dumpfile("fifo_extram36_tb.vcd");
//   initial $dumpvars(0,fifo_extram36_tb);
   initial $timeformat(-9, 0, " ns", 10);
   initial task_SSRAMMonitor;
   
   initial
     begin
	@(negedge rst);
	#40000;
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
//	ReadFromFIFO36;
	task_ReadFIFO36;
	
     end

   integer i;
   
   initial
     begin
	@(negedge rst);
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
	task_WriteFIFO36;
	@(posedge clk);
	@(posedge clk);
	@(posedge clk);
//	PutPacketInFIFO36(32'hA0B0C0D0,12);
	@(posedge clk);
	@(posedge clk);
	#10000;
	@(posedge clk);
//	PutPacketInFIFO36(32'hE0F0A0B0,36);
	@(posedge clk);
	@(posedge clk);
	task_WriteFIFO36;
	@(posedge clk);
	@(posedge clk);
	#10000;
	@(posedge clk);
	@(posedge clk);
	task_WriteFIFO36;
//	@(posedge clk);
//	#30000;
//	@(posedge clk);
//	@(posedge clk);
	task_WriteFIFO36;
//	@(posedge clk);
//	#30000;
//	@(posedge clk);
//	@(posedge clk);
	task_WriteFIFO36;
//	@(posedge clk);
//	#30000;
//	@(posedge clk);
//	@(posedge clk);
	task_WriteFIFO36;
	@(posedge clk);
	#10000;
	@(posedge clk);
	@(posedge clk);
	task_WriteFIFO36;
	for (i=0; i<8192; i = i+1) begin
	   @(posedge clk);
	   #10000;
	   @(posedge clk);
	   @(posedge clk);
	   task_WriteFIFO36;
	   @(posedge clk);
	end

//	$dumpvars(0,fifo_extram36_tb);
	@(posedge clk);
	task_WriteFIFO36;
	@(posedge clk);

	#100000000;
	$finish;
	
     end
  */

   initial
     begin
	@(negedge rst);
	f36_occ      <= 0;
	repeat (100)
	  @(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= 32'h10203040;
	f36_sof      <= 1;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= f36_data + 32'h01010101;
	f36_sof      <= 0;
	f36_eof      <= 0;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 1;
	f36_data     <= 32'h1F2F3F4F;
	f36_sof      <= 0;
	f36_eof      <= 1;
	@(posedge clk);
	@(posedge clk);
	src_rdy_f36i <= 0;
	
	
	
     end
   
//   initial #500000 $finish;
endmodule // fifo_extram_tb
