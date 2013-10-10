
module tx_responder
  #(parameter BASE = 0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input ack_or_error, input packet_consumed,
    input [11:0] seqnum,
    input [63:0] error_code,
    input [31:0] sid,
    
    input [63:0] vita_time,
    output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   reg [11:0] 	  seqnum_int;

   always @(posedge clk)
     if(packet_consumed)
       seqnum_int <= seqnum;
   
   wire 	  trigger_fc, trigger_ctxt;
   wire [95:0] 	  msg_data = { sid[15:0], sid[31:16], (ack_or_error ? error_code : {32'h0,20'h0,seqnum_int}) };
   wire [95:0] 	  ctxt_data;

   reg [11:0] 	  reply_seqnum;
   wire 	  done;
   
   always @(posedge clk)
     if(reset | clear)
       reply_seqnum <= 12'd0;
     else if(done)
       reply_seqnum <= reply_seqnum + 12'd1;
   
   trigger_context_pkt #(.BASE(BASE)) trig
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .packet_consumed(packet_consumed), .trigger(trigger_fc));

   axi_fifo_short #(.WIDTH(64+32)) ack_queue
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(msg_data), .i_tvalid(ack_or_error | trigger_fc), .i_tready(),
      .o_tdata(ctxt_data), .o_tvalid(trigger_ctxt), .o_tready(done),
      .space(), .occupied());
   
   context_packet_gen ack_err_gen
     (.clk(clk), .reset(reset), .clear(clear),
      .trigger(trigger_ctxt), .seqnum(reply_seqnum), .sid(ctxt_data[95:64]),
      .body(ctxt_data[63:0]), .vita_time(vita_time),
      .done(done),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));
      
endmodule // tx_responder
