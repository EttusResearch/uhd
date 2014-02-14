//
// Copyright 2012 Ettus Research LLC
//


//
// This module is instantiated in parallel with a FIFO with AXI4-STREAM interfaces.
// It tracks how many complete packets are contained within the FIFO, and also indicates
// when the first word of a packet is presented on the FIFO outputs.
//

 
module monitor_axi_fifo
  #(
    parameter COUNT_BITS=8
    )
  (
   input clk,
   input reset,
   input clear,
   // Monitored FIFO signals
   input i_tvalid,
   input i_tready,
   input i_tlast,
   input o_tvalid,
   input o_tready,
   input o_tlast,
   // FIFO status outputs
   output reg [COUNT_BITS-1:0] pkt_count, // Exact whole packet count
   output pkt_present // Flags any whole packets present
 
   );

   localparam WAIT_SOF = 0;
   localparam WAIT_EOF = 1;
    
   
   reg        in_state, out_state;
   reg        pause_tx;
                
   //
   // Count packets arriving into large FIFO
   //
   always @(posedge clk)
     if (reset | clear) begin
        in_state <= WAIT_SOF;
     end else
       case(in_state)
	 //
	 // After RESET or the EOF of previous packet, the first cycle with
	 // input valid and input ready asserted is the SOF.
	 //
         WAIT_SOF: 
           if (i_tvalid && i_tready) begin
              in_state <= WAIT_EOF;
           end else begin
              in_state <= WAIT_SOF;
           end
	 //
	 // EOF is signalled by the assertion i_tlast whilst input valid and ready are asserted.
	 //
         WAIT_EOF: 
           if (i_tlast && i_tvalid && i_tready) begin
              in_state <= WAIT_SOF;
           end else begin
              in_state <= WAIT_EOF;
           end
       endcase // case(in_state)
   


   //
   // Count packets leaving large FIFO
   //
   always @(posedge clk)
     if (reset | clear) begin
        out_state <= WAIT_SOF;
     end else
       case(out_state)
	 //
	 // After RESET or the EOF of previous packet, the first cycle with
	 // output valid and output ready asserted is the SOF.
	 //
         WAIT_SOF: 
           if (o_tvalid && o_tready) begin
              out_state <= WAIT_EOF;
           end else begin
              out_state <= WAIT_SOF;
           end
	 //
	 // EOF is signalled by o_tlast asserted whilst output valid and ready asserted.
	 //
         WAIT_EOF: 
           if (o_tlast && o_tvalid && o_tready) begin
              out_state <= WAIT_SOF;
           end else begin
              out_state <= WAIT_EOF;
           end
       endcase // case(in_state)
   

   //
   // Count packets in FIFO.
   // No protection on counter wrap, 
   // unclear how to gracefully deal with it.
   // Perhaps generate Error IRQ so that S/W could clean up?
   // Configure so that the pkt_count is ample for the application.
   //
   always @(posedge clk)
     if (reset | clear)
       pkt_count <= 0;
     else if (((out_state==WAIT_EOF) && o_tlast && o_tvalid && o_tready ) &&
	      ((in_state==WAIT_EOF) && i_tlast && i_tvalid && i_tready))
       pkt_count <= pkt_count;
     else if ((out_state==WAIT_EOF) && o_tlast && o_tvalid && o_tready)
       pkt_count <= pkt_count - 1;
     else if ((in_state==WAIT_EOF)  && i_tlast && i_tvalid && i_tready)
       pkt_count <= pkt_count + 1;

   // Non-zero packet count indicates packet(s) present.
   assign pkt_present = |pkt_count;
 
endmodule // count_tx_packets
