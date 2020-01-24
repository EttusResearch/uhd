//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


// HALT brings RX to an idle state as quickly as possible if RX is running
// without running the risk of leaving a packet fragment in downstream FIFO's.
// HALT also flushes all remaining pending commands in the commmand FIFO.
// Unlike STOP, HALT doesn't ever create an ERROR packet.


module new_rx_control
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input [63:0] vita_time,
    
    // DDC connections
    output run, output eob,
    input strobe, input full,
    input [11:0] seqnum,
    input [31:0] sid,

    output [63:0] err_tdata, output err_tlast, output err_tvalid, input err_tready,
    output reg [3:0] ibs_state,
    output [31:0] debug
    );

   wire [31:0] 	  command_i;
   wire [63:0] 	  time_i;
   wire 	  store_command;

   wire 	  send_imm, chain, reload, stop;
   wire [27:0] 	  numlines;
   wire [63:0] 	  rcvtime;

   wire 	  now, early, late;
   wire 	  command_valid;
   reg 		  command_ready;
   
   reg 		  chain_sav, reload_sav;

   
   reg 		  clear_halt;
   reg 		  halt;
   wire 	  set_halt;
    
   reg [63:0] 	  err_tdata_int;
   wire 	  err_tlast_int;
   wire 	  err_tvalid_int;
   wire 	  err_tready_int;
   

   setting_reg #(.my_addr(BASE)) sr_cmd
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(command_i),.changed());

   setting_reg #(.my_addr(BASE+1)) sr_time_h
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(time_i[63:32]),.changed());
   
   setting_reg #(.my_addr(BASE+2)) sr_time_l
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(time_i[31:0]),.changed(store_command));

   setting_reg #(.my_addr(BASE+3)) sr_rx_halt
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(set_halt));

   always @(posedge clk)
     if (reset | clear | clear_halt)
       halt <= 1'b0;
     else
       halt <= set_halt;
   
   
   axi_fifo_short #(.WIDTH(96)) commandfifo
     (.clk(clk),.reset(reset),.clear(clear | clear_halt),
      .i_tdata({command_i,time_i}), .i_tvalid(store_command), .i_tready(),
      .o_tdata({send_imm,chain,reload,stop,numlines,rcvtime}),
      .o_tvalid(command_valid), .o_tready(command_ready),
      .occupied(), .space() );

   time_compare 
     time_compare (.clk(clk), .reset(reset), .time_now(vita_time), .trigger_time(rcvtime), 
		   .now(now), .early(early), .late(late), .too_early());

   localparam IBS_IDLE         = 0;
   localparam IBS_RUNNING      = 1;
   
   localparam IBS_OVERRUN      = 2;
   localparam IBS_OVR_TIME     = 3;
   localparam IBS_OVR_DATA     = 4;
   
   localparam IBS_BROKENCHAIN  = 5;
   localparam IBS_BRK_TIME     = 6;
   localparam IBS_BRK_DATA     = 7;
      
   localparam IBS_LATECMD      = 8;
   localparam IBS_LATE_TIME    = 9;
   localparam IBS_LATE_DATA    = 10;
   
   localparam IBS_ZEROLEN      = 11;
   localparam IBS_ZERO_TIME    = 12;
   localparam IBS_ZERO_DATA    = 13;
   
   reg [27:0] 	  lines_left, repeat_lines;

   
   always @(posedge clk)
     if(reset | clear)
       begin
	  ibs_state <= IBS_IDLE;
	  chain_sav <= 1'b0;
	  reload_sav <= 1'b0;
	  clear_halt <= 1'b0;
       end
     else
       case (ibs_state)
	 IBS_IDLE : begin
	   clear_halt <= 1'b0; // Incase we got here through a HALT.
	   if (command_valid)
	     // There is a valid command to pop from FIFO.
	     if (stop) begin
		// Stop bit set in this command, go idle.
		ibs_state <= IBS_IDLE;//IBS_ZEROLEN;
	     end else if (late & ~send_imm) begin
		// Got this command later than its execution time.
		ibs_state <= IBS_LATECMD;
	     end else if (now | send_imm) begin
		// Either its time to run this command or it should run immediately without a time.
		ibs_state <= IBS_RUNNING;
		lines_left <= numlines;
		repeat_lines <= numlines;
		chain_sav <= chain;
		reload_sav <= reload;
	     end
	 end // case: IBS_IDLE
	 
	 IBS_RUNNING : begin 
	    if (strobe) begin 
	       if (full) begin
		  // Framing FIFO is full and we have just overrun.
		  ibs_state <= IBS_OVERRUN;
	       end else if (lines_left == 1) begin
		  // Provide Halt mechanism used to bring RX into known IDLE state
		  // at re-initialization.
		  if (halt) begin
		     ibs_state <= IBS_IDLE;
		     clear_halt <= 1'b1;
		  end else if (chain_sav) begin
		     // If chain_sav is true then execute the next command now this one finished.
		     if (command_valid) begin
			lines_left <= numlines;
			repeat_lines <= numlines;
			chain_sav <= chain;
			reload_sav <= reload;
			// If the new command includes stop then go idle.
			if (stop) begin
			   ibs_state <= IBS_IDLE;
			end
		     end else if (reload_sav) begin
			// There is no new command to pop from FIFO so re-run previous command.
			lines_left <= repeat_lines;
		     end else begin
			// Chain has been broken, no commands left in FIFO and reload not set.
			ibs_state <= IBS_BROKENCHAIN;
		     end
		  end else begin // if (chain_sav)
		     // Chain is not true, so don't look for new command, instead go idle.
		     ibs_state <= IBS_IDLE;
		  end
	       end else begin // if (lines_left == 1)
		  // Still counting down lines in current command.
		  lines_left <= lines_left - 28'd1;
	       end
	    end // if (strobe)
	 end // case: IBS_RUNNING
	 
	 
	IBS_OVERRUN: if(err_tready_int) ibs_state <= IBS_OVR_TIME;
	IBS_OVR_TIME: if(err_tready_int) ibs_state <= IBS_OVR_DATA;
	IBS_OVR_DATA: if(err_tready_int) ibs_state <= IBS_IDLE;

	IBS_BROKENCHAIN: if(err_tready_int) ibs_state <= IBS_BRK_TIME;
	IBS_BRK_TIME: if(err_tready_int) ibs_state <= IBS_BRK_DATA;
	IBS_BRK_DATA: if(err_tready_int) ibs_state <= IBS_IDLE;

	IBS_LATECMD: if(err_tready_int) ibs_state <= IBS_LATE_TIME;
	IBS_LATE_TIME: if(err_tready_int) ibs_state <= IBS_LATE_DATA;
	IBS_LATE_DATA: if(err_tready_int) ibs_state <= IBS_IDLE;

	IBS_ZEROLEN: if(err_tready_int) ibs_state <= IBS_ZERO_TIME;
	IBS_ZERO_TIME: if(err_tready_int) ibs_state <= IBS_ZERO_DATA;
	IBS_ZERO_DATA: if(err_tready_int) ibs_state <= IBS_IDLE;

	default: ibs_state <= IBS_IDLE;

       endcase // case (ibs_state)

   always @*
     case(ibs_state)
       IBS_IDLE    : command_ready <= stop | late | now | send_imm;
       IBS_RUNNING : command_ready <= strobe & (lines_left == 1) & chain_sav;
       default     : command_ready <= 1'b0;
     endcase // case (ibs_state)

   assign run = (ibs_state == IBS_RUNNING);
   assign eob = strobe && (lines_left == 1) && ( !chain_sav || (command_valid && stop) || (!command_valid && !reload_sav) || halt);

   always @*
     case (ibs_state)
       IBS_OVERRUN : err_tdata_int <= { 4'hA, seqnum, 16'd24, sid };
       IBS_OVR_TIME : err_tdata_int <= vita_time;
       IBS_OVR_DATA : err_tdata_int <= {32'h8, 32'b0};

       IBS_BROKENCHAIN : err_tdata_int <= { 4'hA, seqnum, 16'd24, sid };
       IBS_BRK_TIME : err_tdata_int <= vita_time;
       IBS_BRK_DATA : err_tdata_int <= {32'h4, 32'b0};

       IBS_LATECMD : err_tdata_int <= { 4'hA, seqnum, 16'd24, sid };
       IBS_LATE_TIME : err_tdata_int <= vita_time;
       IBS_LATE_DATA : err_tdata_int <= {32'h2, 32'b0};

       IBS_ZEROLEN : err_tdata_int <= { 4'hA, seqnum, 16'd24, sid };
       IBS_ZERO_TIME : err_tdata_int <= vita_time;
       IBS_ZERO_DATA : err_tdata_int <= {32'hd, 32'b0};

       default : err_tdata_int <= {32'he, 32'b0};
     endcase // case (ibs_state)
   
   assign err_tlast_int = (ibs_state == IBS_OVR_DATA) 
     | (ibs_state == IBS_BRK_DATA) 
       | (ibs_state == IBS_LATE_DATA) 
	 | (ibs_state == IBS_ZERO_DATA);
   
   assign err_tvalid_int = ibs_state >= IBS_OVERRUN;


   assign debug[27:0] = lines_left;
   assign debug[28] = stop;
   assign debug[29] = halt;
   assign debug[30] = command_valid;
   assign debug[31] = command_ready;
   
   axi_fifo_short #(.WIDTH(65)) output_fifo
     (
      .clk(clk), .reset(reset), .clear(clear),
      .i_tdata({err_tlast_int,err_tdata_int}), .i_tvalid(err_tvalid_int), .i_tready(err_tready_int),
      .o_tdata({err_tlast,err_tdata}), .o_tvalid(err_tvalid), .o_tready(err_tready),
      .space(), .occupied()
      );


   
endmodule // new_rx_control
