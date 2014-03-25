//
// Copyright 2013 Ettus Research LLC
//

//
//
// Provide required handshake to Opencores XGE MAC to initiate Rx of one available packet
// 
//


module xge_handshake
  (
   input clk,
   input reset,
   output reg pkt_rx_ren,
   input pkt_rx_avail,
   input pkt_rx_eop
   );

   localparam IDLE=0;
   localparam RX=1;

   reg 	      state;
   

   always @(posedge clk)
     if (reset) begin
	pkt_rx_ren <= 0;
	state <= IDLE;
     end else begin
	case (state)
	  //
	  // Wait for pkt_rx_avail to be asserted, then assert pkt_rx_ren next cycle
	  //
	  IDLE: begin
	     if (pkt_rx_avail) begin
		pkt_rx_ren <= 1;
		state <= RX;
	     end else begin
		pkt_rx_ren <= 0;
		state <= IDLE;
	     end
	  end
	  //
	  // Keep pkt_rx_ren asserted until EOF received.
	  //
	  RX: begin
	     if (pkt_rx_eop) begin
		pkt_rx_ren <= 0;
		state <= IDLE;
	     end else begin
		pkt_rx_ren <= 1;
		state <= RX;
	     end
	  end   
	     
	endcase // case(state)
     end // else: !if(reset)
endmodule // xge_handshake
