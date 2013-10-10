//
// Copyright 2012 Ettus Research LLC
//


//
// This module forms the qualification engine for a single master as
// part of a larger arbitration engine for a slave. It would typically
// be instantiated from arb_select_master.v to form a complete arbitor solution.
//

module arb_qualify_master
  #(
    parameter WIDTH=16  // Bit width of destination field.
    )
    (  
       input clk,
       input reset,
       input clear,     
       // Header signals
       input [WIDTH-1:0] header,
       input header_valid,
       // Slave Confg Signals
       input [WIDTH-1:0] slave_addr,
       input [WIDTH-1:0] slave_mask,
       input slave_valid,
       // Arbitration flags
       output reg master_valid,
       input master_ack
       );

   localparam WAIT_HEADER_VALID = 0;
   localparam MATCH = 1;
   localparam WAIT_HEADER_NOT_VALID = 2;
   
   
   reg [1:0] state, next_state;

   
   // Does masked slave address match header field for dest from master?
   assign    header_match = ((header & slave_mask) == (slave_addr & slave_mask)) && slave_valid;
   
   
   always @(posedge clk)
     if (reset | clear) begin
        state <= WAIT_HEADER_VALID;
	master_valid <= 0;
     end else
       begin	  
	  case(state)
	    //
	    // Wait here until Masters FIFO presents a valid header word.
	    //
	    WAIT_HEADER_VALID: begin
	       if (header_valid)
		 if (header_match) begin
		    state <= MATCH;
		    master_valid <= 1;
		 end else
		   next_state <= WAIT_HEADER_NOT_VALID;
	    end
	    //
	    // There should only ever be one match across various arbitors
	    // if they are configured correctly and since the backing FIFO in the
	    // master should not start to drain until the arbitration is won
	    // by that master, master_ack should always preceed de-assertion of
	    // header_valid so we don't check for the other order of deassertion.
	    //
	    MATCH: begin
	       if (master_ack) begin
		  master_valid <= 0;
		  state <= WAIT_HEADER_NOT_VALID;
	       end
	    end
	    //
	    // Wait here until this master starts to drain this packet from his FIFO.
	    //
	    WAIT_HEADER_NOT_VALID: begin
	       if (!header_valid) begin
		  state <= WAIT_HEADER_VALID;
	       end
	    end
	  endcase // case(state)
       end // else: !if(reset | clear)

endmodule // arb_qualify_master

	 