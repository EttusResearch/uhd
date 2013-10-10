
// source_flow_control.v
//
//  This block passes the in_* AXI port to the out_* AXI port only when it has
//   enough flow control credits.  Data is held when there are not enough credits.
//  Credits are replenished with extension context packets which update the 
//   last_consumed packet register.  Max credits are controlled by settings regs.
//  The 2nd line of the packet contains the sequence number in the low 12 bits.
//  These packets should not have a time value, but if they do it will be ignored.

module source_flow_control
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] fc_tdata, input fc_tlast, input fc_tvalid, output fc_tready,
    input [63:0] in_tdata, input in_tlast, input in_tvalid, output in_tready,
    output [63:0] out_tdata, output out_tlast, output out_tvalid, input out_tready);
   
   reg [31:0] 	  last_seqnum_consumed;
   wire [31:0] 	  window_size;
   wire [31:0] 	  go_until_seqnum = last_seqnum_consumed + window_size + 1;
   reg [31:0] 	  current_seqnum;
   wire 	  window_reset;
   wire 	  window_enable;
   wire 	  setting_changed;
   
   setting_reg #(.my_addr(BASE)) sr_window_size
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
      .out(window_size),.changed(setting_changed));

   setting_reg #(.my_addr(BASE+1), .width(1)) sr_window_enable
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
      .out(window_enable),.changed(window_reset));

   reg 		  go;
   reg [1:0] 	  sfc_state;
  
   
   localparam SFC_HEAD = 2'd0;
   localparam SFC_TIME = 2'd1;
   localparam SFC_BODY = 2'd2;
   localparam SFC_DUMP = 2'd3;

   always @(posedge clk)
     if(reset | clear | window_reset)
       begin
	  last_seqnum_consumed <= 32'hFFFFFFFF;
	  sfc_state <= SFC_HEAD;
       end
     else
       if(fc_tvalid & fc_tready)
	 case(sfc_state)
	   SFC_HEAD :
	     if(fc_tlast)
	       sfc_state <= SFC_HEAD; // Error. CHDR packet with only a header is an error.
	     else if(~fc_tdata[63])   // Is this NOT an extension context packet?
	       sfc_state <= SFC_DUMP; // Error. Only extension context packets should come in on this interface.
	     else if(fc_tdata[61])    // Does this packet have time?
	       sfc_state <= SFC_TIME;
	     else
	       sfc_state <= SFC_BODY;
	   
	   SFC_TIME :
	     if(fc_tlast)
	       sfc_state <= SFC_HEAD; // Error, CHDR packet with only header and time is an error.
	     else
	       sfc_state <= SFC_BODY;
	   
	   SFC_BODY :
	     begin
		last_seqnum_consumed <= fc_tdata[31:0]; // Sequence number is in lower 32bits.
		if(fc_tlast)
		  sfc_state <= SFC_HEAD;
		else
		  sfc_state <= SFC_DUMP; // Error. Not expecting any more data in a CHDR packet.
	     end

	   SFC_DUMP :   // shouldn't ever need to be here, this is an error condition
	     if(fc_tlast)
	       sfc_state <= SFC_HEAD;
	 endcase // case (sfc_state)
      
   assign fc_tready = 1'b1;  // Always consume FC -- FIXME Even if we are getting reset?
   assign out_tdata = in_tdata; // CHDR data flows through combinatorially.
   assign out_tlast = in_tlast;
   assign in_tready = go ? out_tready : 1'b0;
   assign out_tvalid = go ? in_tvalid : 1'b0;

   //
   // Each time we recieve the end of an IF data packet increment the current_seqnum.
   // We bravely assume that no packets go missing...or at least that they will be detected elsewhere
   // and then handled appropriately.
   // The SEQNUM needs to be initialized every time we start a new stream. In new_rx_framer this is done
   // as a side effect of writing a new SID value to the setting reg.
   //
   // By incrementing current_seqnum on the last signal we get the nice effect that packet flow is 
   // always suspended between packets rather than within a packet.
   //
   always @(posedge clk)
     if(reset | clear | window_reset)
       current_seqnum <= 0;
     else if (in_tvalid && in_tready && in_tlast)
       current_seqnum <= current_seqnum + 1;

   always @(posedge clk)
     if(reset | clear)
       go <= 1'b0;
     else
       if(~window_enable)
	 go <= 1'b1;
       else 
	 case(go)
	   1'b0 :
	     // This test assumes the host is well behaved in sending good numbers for packets consumed
	     // and that current_seqnum increments always by 1 only.
	     // This way wraps are dealt with without a large logic penalty.
	     if (in_tvalid & (go_until_seqnum - current_seqnum != 0))
	     // 	     if(in_tvalid & (go_until_seqnum > current_seqnum))  // FIXME will need to handle wrap of 32-bit seqnum
	       go <= 1'b1;
	   
	   1'b1 :
	     if(in_tvalid & in_tready & in_tlast)
	       go <= 1'b0;
	 endcase // case (go)

   
endmodule // source_flow_control
