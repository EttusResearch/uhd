
module vita_rx_chain
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] vita_time, output overrun,
    input [31:0] sample, output run, input strobe,
    output [35:0] rx_data_o, output rx_src_rdy_o, input rx_dst_rdy_i,
    output [31:0] debug );
       
   wire [99:0] sample_data;
   wire        sample_dst_rdy, sample_src_rdy;
   wire [31:0] vrc_debug, vrf_debug;
   
   vita_rx_control #(.BASE(BASE), .WIDTH(32)) vita_rx_control
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time), .overrun(overrun),
      .sample(sample), .run(run), .strobe(strobe),
      .sample_fifo_o(sample_data), .sample_fifo_dst_rdy_i(sample_dst_rdy), .sample_fifo_src_rdy_o(sample_src_rdy),
      .debug_rx(vrc_debug));
   
   vita_rx_framer #(.BASE(BASE), .MAXCHAN(1)) vita_rx_framer
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .sample_fifo_i(sample_data), .sample_fifo_dst_rdy_o(sample_dst_rdy), .sample_fifo_src_rdy_i(sample_src_rdy),
      .data_o(rx_data_o), .src_rdy_o(rx_src_rdy_o), .dst_rdy_i(rx_dst_rdy_i),
      .fifo_occupied(), .fifo_full(), .fifo_empty(),
      .debug_rx(vrf_debug) );

   assign debug = vrc_debug | vrf_debug;
   
endmodule // vita_rx_chain
