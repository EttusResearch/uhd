
module vita_tx_chain
  #(parameter SR_TX_CTRL=0,
    parameter SR_TX_DSP=0)
   (input dsp_clk, input dsp_rst,
    input set_stb_dsp, input [7:0] set_addr_dsp, input [31:0] set_data_dsp,
    input [63:0] vita_time,
    input [35:0] tx_data_i, input tx_src_rdy_i, output tx_dst_rdy_o,
    output [15:0] dac_a, output [15:0] dac_b,
    output underrun, output run_tx,
    output [31:0] debug);
   
   wire [31:0] 	  debug_vtc, debug_vtd, debug_tx_dsp;
   wire [99:0] 	  tx1_data;
   wire 	  tx1_src_rdy, tx1_dst_rdy;
   wire 	  clear_vita;
   wire [31:0] 	  sample_tx;
   
   vita_tx_deframer #(.BASE(SR_TX_CTRL), .MAXCHAN(1)) vita_tx_deframer
     (.clk(dsp_clk), .reset(dsp_rst), .clear(clear_vita),
      .set_stb(set_stb_dsp),.set_addr(set_addr_dsp),.set_data(set_data_dsp),
      .data_i(tx_data_i), .src_rdy_i(tx_src_rdy_i), .dst_rdy_o(tx_dst_rdy_o),
      .sample_fifo_o(tx1_data), .sample_fifo_src_rdy_o(tx1_src_rdy), .sample_fifo_dst_rdy_i(tx1_dst_rdy),
      .debug(debug_vtd) );

   vita_tx_control #(.BASE(SR_TX_CTRL), .WIDTH(32)) vita_tx_control
     (.clk(dsp_clk), .reset(dsp_rst), .clear(clear_vita),
      .set_stb(set_stb_dsp),.set_addr(set_addr_dsp),.set_data(set_data_dsp),
      .vita_time(vita_time),.underrun(underrun),
      .sample_fifo_i(tx1_data), .sample_fifo_src_rdy_i(tx1_src_rdy), .sample_fifo_dst_rdy_o(tx1_dst_rdy),
      .sample(sample_tx), .run(run_tx), .strobe(strobe_tx),
      .debug(debug_vtc) );
   
   dsp_core_tx #(.BASE(SR_TX_DSP)) dsp_core_tx
     (.clk(dsp_clk),.rst(dsp_rst),
      .set_stb(set_stb_dsp),.set_addr(set_addr_dsp),.set_data(set_data_dsp),
      .sample(sample_tx), .run(run_tx), .strobe(strobe_tx),
      .dac_a(dac_a),.dac_b(dac_b),
      .debug(debug_tx_dsp) );

   assign debug = debug_vtc | debug_vtd;
   
endmodule // vita_tx_chain
