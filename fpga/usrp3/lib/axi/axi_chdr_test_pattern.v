//
// Synthesizable test pattern generators and checkers
// for CHDR that can be used to test transparent blocks
// (FIFOs, switches, etc)
//

//`define MTU 8192
`define MTU 1536

module axi_chdr_test_pattern
  (
   input clk,
   input reset,
  
   //
   // CHDR friendly AXI stream input
   //
   output reg [63:0] i_tdata,
   output reg i_tlast,
   output reg i_tvalid,
   input  wire i_tready,
   //
   // CHDR friendly AXI Stream output
   //
   input wire [63:0] o_tdata,
   input wire o_tlast,
   input wire o_tvalid,
   output reg o_tready,
   //
   // Test flags
   //
   input start,
   input [15:0] control,
   output reg fail,
   output reg done
   );

   wire [7:0]      bist_rx_delay = control[7:0];
   wire [7:0]      bist_tx_delay = control[15:8];
   

   reg [15:0] tx_count, rx_count;
   reg [15:0] tx_data, rx_data;
   reg [7:0]  tx_delay, rx_delay;
   

   localparam TX_IDLE = 0;
   localparam TX_START = 1;
   localparam TX_ACTIVE = 2;
   localparam TX_GAP = 3;
   localparam TX_DONE = 4;
   localparam TX_WAIT = 5;

   localparam RX_IDLE = 0;
   localparam RX_ACTIVE = 1;
   localparam RX_FAIL = 2;
   localparam RX_DONE = 3;
   localparam RX_WAIT = 4;
      
   reg [2:0]  tx_state, rx_state;
 
   //
   // Transmitter
   //
   always @(posedge clk)
     if (reset)
       begin
	  tx_delay <= 0; 
	  tx_count <= 8;
	  tx_data <= 0;
	  i_tdata <= 64'h0;
	  i_tlast <= 1'b0;
	  i_tvalid <= 1'b0;
	  tx_state <= TX_IDLE;
       end
     else
       begin
	  case(tx_state)
	    TX_IDLE: begin
	       tx_delay <= 0; 
	       i_tdata <= 64'h0;
	       i_tlast <= 1'b0;
	       i_tvalid <= 1'b0;
	       tx_data <= 0;
	       tx_count <= 4;
	       // Run whilst start asserted.
	       if (start) begin
		  tx_state <= TX_START;		  
		  // ....Go back to initialized state if start deasserted.
	       end else begin		 		  
		  tx_state <= TX_IDLE;
	       end		  
	    end // case: TX_IDLE

	    //
	    // START signal is asserted.
	    // Now need to start transmiting a packet.
	    //
	    TX_START: begin
	       // At the next clock edge drive first beat of new packet onto HDR bus.
	       i_tlast <= 1'b0;
	       i_tvalid <= 1'b1;
	       tx_data <= tx_data + 4;
	       // i_tdata <= {tx_data,tx_data+16'd1,tx_data+16'd2,tx_data+16'd3};
	       i_tdata <= {4{(tx_data[2]?16'hffff:16'h0000)^tx_data[15:0]}};
	       tx_state <= TX_ACTIVE;
	       
	    end
	    
	    //
	    // Valid data is (already) being driven onto the CHDR bus.
	    // i_tlast may also be driven asserted if current data count has reached EOP.
	    // Watch i_tready to see when it's consumed.
	    // When packets are consumed increment data counter or transition state if
	    // EOP has sucsesfully concluded.
	    //
	    TX_ACTIVE: begin
	       i_tvalid <= 1'b1; // Always assert tvalid
	       if (i_tready) begin
		  
//		  i_tdata <= {tx_data,tx_data+16'd1,tx_data+16'd2,tx_data+16'd3};
		  i_tdata <= {4{(tx_data[2]?16'hffff:16'h0000)^tx_data[15:0]}};
		  // Will this next beat be the last in a packet?
		  if (tx_data == tx_count) begin
		     tx_data <= 0;
		     i_tlast <= 1'b1;
		     tx_state <= TX_GAP;
		  end else begin
		     tx_data <= tx_data + 4;
		     i_tlast <= 1'b0;
		     tx_state <= TX_ACTIVE;
		  end		  
	       end else begin
		  // Keep driving all CHDR bus signals as-is until i_tready is asserted.
		  tx_state <= TX_ACTIVE;
	       end
	    end // case: TX_ACTIVE
	    //
	    // Force an inter-packet gap between packets in a BIST sequence where tvalid is driven low.
	    // As we leave this state check if all packets in BIST sequence have been generated yet,
	    // and if so go to done state.
	    //
	    TX_GAP: begin
	       if (i_tready) begin
		  i_tvalid <= 1'b0;
		  i_tdata <= 64'h0;
		  i_tlast <= 1'b0;
		  tx_count <= tx_count + 4;
		  
		  if (tx_count < `MTU) begin
		     tx_state <= TX_WAIT;
		     tx_delay <= bist_tx_delay;		  
		  end else
		    tx_state <= TX_DONE;
	       end else begin // if (i_tready)
		  tx_state <= TX_GAP;
	       end
	    end // case: TX_GAP
	    //
	    // Simulate inter packet gap in real UHD system
	    TX_WAIT: begin
	       if (tx_delay == 0)
		 tx_state <= TX_START;
	       else begin
		  tx_delay <= tx_delay - 1;
		  tx_state <= TX_WAIT;
	       end
	    end
	    
	    //
	    // Complete test pattern BIST sequence has been transmitted. Sit in this
	    // state indefinately if START is taken low, which re-inits the whole BIST solution.
	    // 
	    TX_DONE: begin
	       if (!start) begin
		  tx_state <= TX_DONE;
	       end else begin
		  tx_state <= TX_IDLE;
	       end
	       i_tvalid <= 1'b0;
	       i_tdata <= 64'd0;
	       i_tlast <= 1'b0;
	       
	    end
	  endcase // case (tx_state)
       end

   //
   // Receiver
   //
   always @(posedge clk)
     if (reset)
       begin
	  rx_delay <= 0;
	  rx_count <= 0;
	  rx_data <= 0;
	  o_tready <= 1'b0;
	  rx_state <= RX_IDLE;
	  fail <= 1'b0;
	  done <= 1'b0;
	  
       end
     else begin
	case (rx_state)
	  RX_IDLE: begin
	     rx_delay <= 0;
	     o_tready <= 1'b0;
	     rx_data <= 0;
	     rx_count <= 4;
             fail <= 1'b0;
	     done <= 1'b0;
	     // Not accepting data whilst Idle,
	     // switch to active when packet arrives
	     if (o_tvalid) begin		
	        o_tready <= 1'b1;
		rx_state <= RX_ACTIVE;
	     end else
	       rx_state <= RX_IDLE;	       	    
	  end
	  
	  RX_ACTIVE: begin
	     o_tready <= 1'b1;
	     if (o_tvalid)
//	       if (o_tdata != {rx_data,rx_data+16'd1,rx_data+16'd2,rx_data+16'd3})
	       if (o_tdata != {4{(rx_data[2]?16'hffff:16'h0000)^rx_data[15:0]}})
		 begin
		    $display("o_tdata: %x  !=  expected:  %x @ time: %d",o_tdata,
//			     {rx_data,rx_data+16'd1,rx_data+16'd2,rx_data+16'd3},
			     {4{(rx_data[2]?16'hffff:16'h0000)^rx_data[15:0]}},
			     $time);		    
		    rx_state <= RX_FAIL;
		 end
	       else
		 // Should last be asserted?
		 if (rx_data == rx_count)
		   // ...last not asserted when it should be!
		   if (~(o_tlast===1)) begin
		      $display("o_tlast not asserted when it should be @ time: %d",$time);		     
		      rx_state <= RX_FAIL;
		   end else begin
		      // End of packet, set up to RX next 
		      rx_data <= 0;
		      rx_count <= rx_count + 4;
		      rx_delay <= bist_rx_delay;
		      if (rx_count == `MTU) begin
			 rx_state <= RX_DONE;
		      end else begin
			 rx_state <= RX_WAIT;
		      end
		      o_tready <= 1'b0;
		   end	       
		 else
		   // ...last asserted when it should not be!
		   if (~(o_tlast===0)) begin
		      $display("o_tlast asserted when it should not be @ time: %d",$time);		     
		      rx_state <= RX_FAIL;
		   end else begin
		      // Still in packet body
		      rx_data <= rx_data + 4;
		      rx_delay <= bist_rx_delay;
		      rx_state <= RX_WAIT;
		      o_tready <= 1'b0;
		   end
	     else
	       // Nothing to do this cycle
	       rx_state <= RX_ACTIVE;	       
	  end // case: RX_ACTIVE

	  // To simulate the radio consuming samples at a steady rate set by the decimation
	  // have a programable delay here
	  RX_WAIT: begin
	     if (rx_delay == 0) begin
		rx_state <= RX_ACTIVE;
		o_tready <= 1'b1;
	     end else begin
		rx_delay <= rx_delay - 1;
		rx_state <= RX_WAIT;
	     end	     
	  end
	  
	  
	  RX_FAIL: begin
	     o_tready <= 1'b0;
	     done <= 1'b1;
	     fail <= 1'b1;
	     // If start is deasserted allow BIST logic to reset and rearm
	     if (start)
	       rx_state <= RX_FAIL;
	     else
	       rx_state <= RX_IDLE;
	     
	  end
	  
	  RX_DONE: begin
	     o_tready <= 1'b0;
	     done <= 1'b1;
	     fail <= 1'b0;
	     // If start is asserted allow BIST logic to reset, rearm & restart
	     if (!start)
	       rx_state <= RX_DONE;
	     else
	       rx_state <= RX_IDLE;
	     
	  end
	  
	endcase // case (rx_state)
     end
 
   

endmodule



