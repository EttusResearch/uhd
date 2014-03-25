//
// Synthesizable Tx checker for 10G Ethernet MAC.
// Generates deterministic packets that can be looped back and
// checked for correctness
//
`define IDLE 0
`define MAC1 1
`define MAC2 2
`define PAYLOAD1 3
`define WAIT 4
`define DONE 5


module tx_checker
  (
   input clk156,
   input rst,
   input enable,
   output reg done,
   //
   output reg pkt_tx_val,
   output reg pkt_tx_sop,
   output reg pkt_tx_eop,
   output reg [2:0] pkt_tx_mod,
   output reg [63:0] pkt_tx_data
   );
   
   reg [10:0] 	 payload;
   reg [10:0] 	 count;
   reg [7:0] 	 state;
   reg [9:0] 	 delay;
   
   
   


always @(posedge clk156)
  if (rst)
    begin
       state <= `IDLE;
       count <= 0;
       payload <= 45; // 1 less than ethernet minimum payload size.\
       done <= 0;
       delay <= 0;
       pkt_tx_val <= 0;
       pkt_tx_sop <= 0;
       pkt_tx_eop <= 0;
       pkt_tx_data <= 0;
       pkt_tx_mod <= 0;
    end
  else 
    begin
       state <= state;
       pkt_tx_val <= 0;
       pkt_tx_sop <= 0;
       pkt_tx_eop <= 0;
       pkt_tx_data <= 0;
       pkt_tx_mod <= 0;
       payload <= payload;
       count <= count;
       done <= 0;
       delay <= delay;
       
       
  
       case(state)
	 // Wait in IDLE state until enabled.
	 // As we leave the idle state increment the payload count
	 // so that we have a test pattern that changes each iteration.
	 `IDLE: begin
	    if (enable)
	      begin
		 if (payload == 1500)
		   state <= `DONE;
		 else
		   begin
		      payload <= payload + 1; // Might need to tweak this later..
		      state <= `MAC1;
		   end
	      end
	 end
	 // Assert SOP (Start of Packet) for 1 cycle.
	 // Assert VAL (Tx Valid) for duration of packet.
	 // Put first 8 octets out, including
	 // DST MAC addr and first 2 bytes of SRC MAC.
	 `MAC1: begin
	    pkt_tx_val <= 1;
	    pkt_tx_sop <= 1;
	    pkt_tx_data[63:16] <= 48'h0001020304; // DST MAC
	    pkt_tx_data[15:0] <= 16'h0000; // SRC MAC msb's
	    pkt_tx_mod <= 3'h0; // All octects valid
	    state <= `MAC2;
	 end
	 // SOP now deasserted for rest of packet.
	 // VAL remains asserted.
	 // Tx rest of SRC MAC and ether type then first two data octets.
	 `MAC2: begin
	    pkt_tx_val <= 1;
	    pkt_tx_data[63:32] <= 32'h05060708; // SRC MAC lsb's
	    pkt_tx_data[31:16] <= 16'h88b5;     // Ethertype
	    pkt_tx_data[15:0] <= 16'hBEEF;      // First 2 bytes of payload.
	    pkt_tx_mod <= 3'h0; // All octects valid
	    count <= payload - 2; // Preload counter for this packet
	    state <= `PAYLOAD1;
	 end
	 // Iterate in this state until end of packet.
	 // The first clock cycle in this state, SRC MAC and ETHERTYPE are being Tx'ed due to pipelining
	 `PAYLOAD1: begin
	    pkt_tx_val <= 1;
	    pkt_tx_data <= {8{count[10:3]}}; // Data pattern is 64bit word count value.
	    count <= count - 8;
	    if ((count[10:3] == 0) || (count[10:0] == 8)) // down to 8 or less octects to Tx.
	      begin
		 pkt_tx_mod <= count[2:0];
		 pkt_tx_eop <= 1;
		 state <= `WAIT;
		 delay <= 20; // 20 cycle delay in END state before Tx next packet
	      end
	 end // case: `PAYLOAD1
	 // Because of pipelining EOP is actually asserted in this state
	 // but we are already commited to the end of packet so no decisions need to be made.
	 // Make signals idle ready for next state after.
	 // Delay start of next packet to rate control test.
	 `WAIT:begin
	    delay <= delay - 1;
	    if (delay == 0)
	      state <= `IDLE;	    
	 end
	 // Have now transmitted one packet of every legal size (no jumbo frames)
	 // Stay in this state asserting done flag until reset.
	 `DONE:begin 	   
	    state <= `DONE;
	    done <= 1;
	 end
	 
       endcase // case(state)
      
    end	    
	    
endmodule // tx_checker

   
   