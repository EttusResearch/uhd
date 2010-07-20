
module vita_tx_chain
  #(parameter BASE_CTRL=0,
    parameter BASE_DSP=0,
    parameter REPORT_ERROR=0,
    parameter PROT_ENG_FLAGS=0)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] vita_time,
    input [35:0] tx_data_i, input tx_src_rdy_i, output tx_dst_rdy_o,
    output [35:0] err_data_o, output err_src_rdy_o, input err_dst_rdy_i,
    output [15:0] dac_a, output [15:0] dac_b,
    output underrun, output run,
    output [31:0] debug);

   localparam MAXCHAN = 1;
   localparam FIFOWIDTH = 5+64+(32*MAXCHAN);

   wire [FIFOWIDTH-1:0] tx1_data;
   wire 		tx1_src_rdy, tx1_dst_rdy;
   wire 		clear_vita;
   wire [31:0] 		sample_tx;
   wire [31:0] 		streamid, message;
   wire 		trigger, sent;
   wire [31:0] 		debug_vtc, debug_vtd, debug_tx_dsp;

   wire 		error;
   wire [3:0] 		error_code;
   
   assign underrun = error;
   assign message = {28'h0,error_code};
      
   setting_reg #(.my_addr(BASE_CTRL+2), .at_reset(0)) sr_streamid
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(streamid),.changed());

   vita_tx_deframer #(.BASE(BASE_CTRL), .MAXCHAN(MAXCHAN)) vita_tx_deframer
     (.clk(clk), .reset(reset), .clear(clear_vita),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .data_i(tx_data_i), .src_rdy_i(tx_src_rdy_i), .dst_rdy_o(tx_dst_rdy_o),
      .sample_fifo_o(tx1_data), .sample_fifo_src_rdy_o(tx1_src_rdy), .sample_fifo_dst_rdy_i(tx1_dst_rdy),
      .debug(debug_vtd) );

   vita_tx_control #(.BASE(BASE_CTRL), .WIDTH(32*MAXCHAN)) vita_tx_control
     (.clk(clk), .reset(reset), .clear(clear_vita),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time),.error(error),.error_code(error_code),
      .sample_fifo_i(tx1_data), .sample_fifo_src_rdy_i(tx1_src_rdy), .sample_fifo_dst_rdy_o(tx1_dst_rdy),
      .sample(sample_tx), .run(run), .strobe(strobe_tx),
      .debug(debug_vtc) );
   
   dsp_core_tx #(.BASE(BASE_DSP)) dsp_core_tx
     (.clk(clk),.rst(reset),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .sample(sample_tx), .run(run), .strobe(strobe_tx),
      .dac_a(dac_a),.dac_b(dac_b),
      .debug(debug_tx_dsp) );

   generate
      if(REPORT_ERROR==1)
	gen_context_pkt #(.PROT_ENG_FLAGS(PROT_ENG_FLAGS)) gen_tx_err_pkt
	  (.clk(clk), .reset(reset), .clear(clear_vita),
	   .trigger(error), .sent(), 
	   .streamid(streamid), .vita_time(vita_time), .message(message),
	   .data_o(err_data_o), .src_rdy_o(err_src_rdy_o), .dst_rdy_i(err_dst_rdy_i));
   endgenerate
   
   assign debug = debug_vtc | debug_vtd;
   
endmodule // vita_tx_chain
