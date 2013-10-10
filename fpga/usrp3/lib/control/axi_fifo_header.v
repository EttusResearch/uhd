//
// Copyright 2012 Ettus Research LLC
//


//
// This module is connected to the output port of an AXI4-STREAM FIFO that is used to move packetized data.
// It extracts and indicates the header (first word) of a packet in the FIFO. The header and flag are pipelined
// for timing closure.
//

module axi_fifo_header
  #(
    parameter WIDTH=64  // Bit width of FIFO word.
    )
    (
     input clk,
     input reset,
     input clear,
     // Monitored FIFO signals
     input [WIDTH-1:0] o_tdata,
     input o_tvalid,
     input o_tready,
     input o_tlast,
     input pkt_present,
     // Header signals
     output reg [WIDTH-1:0] header,
     output reg header_valid
     );

   localparam WAIT_SOF = 0;
   localparam WAIT_EOF = 1;
   
   reg 	    out_state;
   

    //
    // Monitor packets leaving FIFO
    //
    always @(posedge clk)
      if (reset | clear) begin
         out_state <= WAIT_SOF;
      end else
	case(out_state)
	  //
	  // After RESET or the EOF of previous packet, the first cycle with
	  // output valid asserted is the SOF and presents the Header word.
	  // The cycle following the concurrent presentation of asserted output 
	  // valid and output ready presents the word following the header.
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
   // Pipeline Header signals
   //
   always @(posedge clk)
     if (reset | clear) begin
	header <= 0;
	header_valid <= 0;
     end else if (o_tvalid && (out_state == WAIT_SOF) && pkt_present) begin
	// Header will remian valid until o_tready is asserted as this will cause a state transition.
	header <= o_tdata;
	header_valid <= 1;
     end else begin
	header_valid <= 0;
     end


endmodule // axi_fifo_header
