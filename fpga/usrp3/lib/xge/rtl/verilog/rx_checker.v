//
// Synthesizable Rx checker for 10G Ethernet MAC.
// Collects recevied packets and checks them against the deterministic expected result
// to verify correct loopback functionality if used with the tx_checker.
//

`define IDLE 0
`define SEARCH 1
`define RECEIVE1 2
`define RECEIVE2 3
`define RECEIVE3 4
`define DONE 5
`define ERROR1 6
`define ERROR2 7
`define ERROR3 8


module rx_checker
  (
   input clk156,
   input rst,
   input enable,
   output reg done,
   output reg correct,
   output reg [1:0] error,
   //
   input pkt_rx_avail,
   input pkt_rx_val,
   input pkt_rx_sop,
   input pkt_rx_eop,
   input [2:0] pkt_rx_mod,
   input [63:0] pkt_rx_data,
   input pkt_rx_err,
   output reg pkt_rx_ren
   );


   reg [10:0] 	 payload;
   reg [10:0] 	 count;
   reg [7:0] 	 state;

   
always @(posedge clk156)
  if (rst)
    begin
       // Reset
       state <= `IDLE;
       done <= 0;
       correct <= 0;
       error <= 0;
       pkt_rx_ren <= 0;
       count <= 0;
       payload <= 45; // 1 less than ethernet minimum payload size.\
    end
  else
    begin
       // Defaults
       state <= state;
       done <= 0;
       correct <= 0;
       error <= 0;
       pkt_rx_ren <= 0;
       count <= count;
       payload <= payload;
       
            
       case(state)
	 // Wait in IDLE state until enabled.
	 // incomming packets will not be detected in this state.
	 `IDLE: begin
	    if (enable)  
	      state <= `SEARCH;
	 end // case: `IDLE
	 //
	 // Search for pkt_rx_avail going asserted to show that a packet is in the MAC's FIFO's.
	 // Then assert pkt_rx_ren back to MAC to start transfer. pkt_rx_ren now remains asserted until 
	 // at least EOP, longer if pkt_rx_avail is still asserted at EOP as back-to-back Rx is possible.
	 // We can come into this state with pkt_rx_ren already enabled for back-to-back Rx cases.
	 //
	 `SEARCH: begin
	      if (pkt_rx_val)
	      state <= `ERROR1;  // Illegal signalling
	    else if (payload == 1500)
	      state <= `DONE;
	    else if (pkt_rx_avail)
	      begin // rx_avail has been asserted, now assert rx_ren to start transfer.
		 payload <= payload + 1; 
		 pkt_rx_ren <= 1;
		 state <= `RECEIVE1;
	      end
	 end
	 //
	 // Now wait for pkt_rx_val and pkt_rx_sop to assert in the same cycle with the first 
	 // 8 octects of a new packet. When asserted check all data bits against expected data.
	 // Go to error states if something doesn't match or work correctly.
	 //
	 `RECEIVE1: begin
	    pkt_rx_ren <= 1;
	    
	    if (pkt_rx_err)
	      state <= `ERROR3; // CRC error from MAC
	    else if (pkt_rx_val && pkt_rx_sop && ~pkt_rx_eop)
	      begin
		 if ((pkt_rx_data[63:16] == 48'h0001020304) && (pkt_rx_data[15:0] == 16'h0000) && (pkt_rx_mod == 3'h0))
		   state <= `RECEIVE2;
		 else
		   state <= `ERROR2; // Data missmatch error	 
	      end	    
	    else if (pkt_rx_val || pkt_rx_sop || pkt_rx_eop) // Error condition
	      begin
		 state <= `ERROR1; // Illegal signalling
	      end
	 end // case: `RECEIVE1
	 //
	 // Check all data bits against expected data.
	 // Go to error states if something doesn't match or work correctly.
	 //
	 `RECEIVE2: begin
	    pkt_rx_ren <= 1;
	    
	    if (pkt_rx_err)
	      state <= `ERROR3; // CRC error from MAC
	    else if (pkt_rx_val && ~pkt_rx_sop && ~pkt_rx_eop)
	      begin
		 if ((pkt_rx_data[63:32] == 32'h05060708) && 
		     (pkt_rx_data[31:16] == 16'h88b5) && 
		     (pkt_rx_data[15:0] == 16'hBEEF) && 
		     (pkt_rx_mod == 3'h0))
		   begin
		      count <= payload - 2; // Preload counter for this packet
		      state <= `RECEIVE3;
		   end
		 else
		   state <= `ERROR2; // Data missmatch error	 
	      end	    
	    else if (~pkt_rx_val || pkt_rx_sop || pkt_rx_eop) // Error condition
	      begin
		 state <= `ERROR1; // Illegal signalling
	      end
	 end // case: `RECEIVE2
	 //
	 // Should now have received both MAC addresses, the ETHERTYPE and first 2 octects of payload.
	 // Check remaining payload whilst looking for end of packet.
	 // Currently don;pt support chained RX of packets, pkt_rx_en will go to 0.
	 // (Remember packets are bigendian)
	 //
	 `RECEIVE3: begin
	    count <= count - 8;
	    
	    if (pkt_rx_err)
	      state <= `ERROR3; // CRC error from MAC
	    else if (pkt_rx_val && ~pkt_rx_sop) 
	      begin
		 case({pkt_rx_eop,pkt_rx_mod})
		   4'b0000: begin
		      if (pkt_rx_data[63:0] == {8{count[10:3]}})
			begin
			   pkt_rx_ren <= 1;
			   state <= `RECEIVE3;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1000: begin
		      if (pkt_rx_data[63:0] == {8{count[10:3]}})
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1001: begin
		      if (pkt_rx_data[63:56] == {1{count[10:3]}})	      
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end	     
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1010: begin
		      if (pkt_rx_data[63:48] == {2{count[10:3]}})		
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1011: begin
		      if (pkt_rx_data[63:40] == {3{count[10:3]}})		
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1100: begin
		      if (pkt_rx_data[63:32] == {4{count[10:3]}})
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1101: begin
		      if (pkt_rx_data[63:24] == {5{count[10:3]}})
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1110: begin
		      if (pkt_rx_data[63:16] == {6{count[10:3]}})
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   4'b1111: begin
		      if (pkt_rx_data[63:8] == {7{count[10:3]}})
			begin
			   pkt_rx_ren <= 0;	     
			   state <= `SEARCH;
			end
		      else
			state <= `ERROR2; // Data missmatch error	 
		   end
		   default: state <= `ERROR1;  // Illegal signalling
		 endcase // case({pkt_rx_eop,pkt_rx_mod})		 
	      end	   
	 end
	 //
	 // Finished. Received and verified full sequence. Now assert corret signal and done.
	 // Stay in this state until reset.
	 //
	 `DONE: begin
	    done <= 1;
	    correct <= 1;
	 end
	 //
	 // Signal protocol error.
	 // Stay in this state until reset.
	 //
	 `ERROR1: begin
	    done <= 1;
	    error <= 1;
	 end
	 //
	 // Data payload of packet did not match reference
	 // Stay in this state until reset.
	 //
	 `ERROR2: begin
	    done <= 1;
	    error <= 2;
	 end
	 //
	 // CRC error reported by MAC
	 // Stay in this state until reset.
	 //
	 `ERROR3: begin
	    done <= 1;
	    error <= 3;
	 end
       endcase
    end

endmodule // rx_checker
