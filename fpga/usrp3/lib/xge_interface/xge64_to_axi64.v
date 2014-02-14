//
// Copyright 2013 Ettus Research LLC
//


// Adds 6 bytes at the beginning of every packet
// This gives us good32/64bit alignment of IP/UDP headers.
//
// The 6 bytes added include an octet passed as a parameter allowing a label to
// be added as metatdata in the header padding. This is typically the ingress
// port to be tagged in the packet as metadata.
//
// bit[65] EOF
// bit[64] SOF
// bit[68:66] occ
//
// This design will break if downstream can not be guarenteed to be ready to accept data.
// XGE MAC expects to be able to stream whole packet with no handshaking.
// We force downstream packet gate to discard packet by signalling error with tlast and
// resynchronizing with upstream.
//

module xge64_to_axi64
  #(parameter LABEL=0)
   (
    input clk, 
    input reset, 
    input clear,
    input [63:0] datain,
    input [2:0] occ,
    input sof,
    input eof,
    input err,
    input valid, 
    output reg [63:0] axis_tdata,
    output reg [3:0] axis_tuser,
    output reg axis_tlast,
    output reg axis_tvalid,   // Signal data avilable to downstream
    input axis_tready
    );

   localparam EMPTY  = 0;
   localparam IN_USE = 1;
   localparam FLUSHING3 = 2;
   localparam FLUSHING4 = 3;
   localparam FLUSHING5 = 4;
   localparam FLUSHING6 = 5;
   localparam FLUSHING7 = 6;
   localparam FLUSHING8 = 7;
   localparam ERROR1 = 8;


   localparam EOF1 = 3'b001;
   localparam EOF2 = 3'b010;
   localparam EOF3 = 3'b011;
   localparam EOF4 = 3'b100;
   localparam EOF5 = 3'b101;
   localparam EOF6 = 3'b110;
   localparam EOF7 = 3'b111;
   localparam EOF8 = 3'b000;

   reg [3:0] state;
   reg 	     err_reg;
   reg [47:0] holding_reg;

   always @(posedge clk)
     if(reset | clear) begin
	state <= EMPTY;
	axis_tdata <= 0;
	holding_reg <= 0;
	axis_tvalid <= 0;	
     end else begin
	// Defaults
	axis_tvalid <= 0;
	axis_tuser <= 0;
	axis_tlast <= 0;
	err_reg <= 0;
	
	case(state)
	  EMPTY: begin
	     if (valid & axis_tready & sof) begin
		// Start of packet should always be received in this state.
		// It should NEVER be possible to get a packet from the MAC with EOF also set in 
		// the first 64 bits so not designed for.
		// Add pad. Store last 6 octets into holding, change state to show data in holding.	
		state <= IN_USE;
		axis_tvalid <= 1;
	     end
	     else if (valid & ~axis_tready)
	       // Assert on this condition, add H/W to deal with overflow later.
	       $display("ERROR: xge64_to_axi64, valid & ~axis_tready");
	     
	     holding_reg <= datain[47:0];
	     axis_tdata[63:56] <= LABEL; // Tag packet with label
	     axis_tdata[55:16] <= 40'h0;
	     axis_tdata[15:0] <= datain[63:48];
	  end
	  
	  IN_USE: begin
	     if (valid & axis_tready & (eof | err)) begin
		// End of packet should always be received in this state.
		// If Error is asserted from MAC, immediate EOF is forced,
		// and the error flag set in tuser. State machine will return to WAIT
		// state and search for new SOF thereby discarding anything left of error packet.
		//
		// In the case of 3 through 8 valid octets in the final 64bits input, 
		// we must run another cycle afterwards since we have 6 more bytes still in holding.
		err_reg <= err;
		holding_reg[47:0] <= datain[47:0];
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tdata[15:0] <= datain[63:48];
		axis_tvalid <= 1;
		
		case(occ[2:0])
		  // 8 valid Octets in last word of packet, finish next cycle
		  0: begin
		     state <= FLUSHING8;
		  end
		  // 7 valid Octets in last word of packet, finish next cycle
		  7: begin
		     state <= FLUSHING7;
		  end
		  // 6 valid octets in last word of packet, finish next cycle
		  6: begin
		     state <= FLUSHING6;
		  end
		  // 5 valid octets in last word of packet, finish next cycle
		  5: begin
		     state <= FLUSHING5;
		  end		    
		  // 4 valid octets in last word of packet, finish next cycle
		  4: begin
		     state <= FLUSHING4;
		  end
		  // 3 valid octets in last word of packet, finish next cycle
		  3: begin
		     state <= FLUSHING3;
		  end
		  // 2 valid octets in last word of packet, finish this cycle
		  2: begin
		     axis_tuser <= {err,EOF8};
		     state <= EMPTY;
		     axis_tlast <= 1;
		  end
		  // 1 valid octets in last word of packet, finish this cycle
		  1: begin
		     axis_tuser <= {err,EOF7};
		     state <= EMPTY;
		     axis_tlast <= 1;
		  end
		endcase // case (occ[2:0])		
	     end // if (valid & axis_tready & eof)
	     else if (valid & axis_tready) begin
		// No EOF indication so in packet payload somewhere still.
		state <= IN_USE;
		holding_reg[47:0] <= datain[47:0];
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tdata[15:0] <= datain[63:48];
		axis_tvalid <= 1;
	     end
	     else if (valid & ~axis_tready) begin
		// Assert on this condition
		$display("ERROR: xge64_to_axi64, valid & ~axis_tready");
		// Keep error state asserted ready for downstream to accept
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end else if (~valid) begin
		// Assert on this condition, don't expect the MAC to ever throtle dataflow intra-packet.
		$display("ERROR: xge64_to_axi64, ~valid ");
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end
	  end // case: IN_USE

	  FLUSHING3: begin
	     if (axis_tready) begin
		// EOF has been received last cycle.
		// Ethernet interframe gap means we don't have to search for back-to-back EOF-SOF here.
		// 1 valid Octets to finish
		state <= EMPTY;
		axis_tlast <= 1;
		axis_tuser <= {err_reg, EOF1};
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tvalid <= 1;
	     end else begin
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end // else: !if(axis_tready)
	  end
	  
	  FLUSHING4: begin
	     if (axis_tready) begin
		// EOF has been received last cycle.
		// Ethernet interframe gap means we don't have to search for back-to-back EOF-SOF here.
		// 2 valid Octets to finish
		state <= EMPTY;
		axis_tlast <= 1;
		axis_tuser <= {err_reg, EOF2};
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tvalid <= 1;
	     end else begin
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end // else: !if(axis_tready)
	  end
	  
	  FLUSHING5: begin
	     if (axis_tready) begin
		// EOF has been received last cycle.
		// Ethernet interframe gap means we don't have to search for back-to-back EOF-SOF here.
		// 3 valid Octets to finish
		state <= EMPTY;
		axis_tlast <= 1;
		axis_tuser <= {err_reg, EOF3};
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tvalid <= 1;
	     end else begin
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end // else: !if(axis_tready)
	  end
	  
	  FLUSHING6: begin
	     if (axis_tready) begin
		// EOF has been received last cycle.
		// Ethernet interframe gap means we don't have to search for back-to-back EOF-SOF here.
		// 4 valid Octets to finish
		state <= EMPTY;
		axis_tlast <= 1;
		axis_tuser <= {err_reg, EOF4};
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tvalid <= 1;
	     end else begin
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end // else: !if(axis_tready)
	  end
	  
	  FLUSHING7: begin
	     if (axis_tready) begin
		// EOF has been received last cycle.
		// Ethernet interframe gap means we don't have to search for back-to-back EOF-SOF here.
		// 5 valid Octets to finish
		state <= EMPTY;
		axis_tlast <= 1;
		axis_tuser <= {err_reg, EOF5};
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tvalid <= 1;
	     end else begin
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end // else: !if(axis_tready)
	  end
	  
	  FLUSHING8: begin
	     if (axis_tready) begin
		// EOF has been received last cycle.
		// Ethernet interframe gap means we don't have to search for back-to-back EOF-SOF here.
		// 6 valid Octets to finish
		state <= EMPTY;
		axis_tlast <= 1;
		axis_tuser <= {err_reg, EOF6};
		axis_tdata[63:16] <= holding_reg[47:0];
		axis_tvalid <= 1;
	     end else begin
		state <= ERROR1;
		axis_tlast <= 1;
		axis_tvalid <= 1;
		axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	     end // else: !if(axis_tready)
	  end

	  ERROR1: begin
	     // We were already actively receiving a packet from the upstream MAC and the downstream
	     // signaled not ready by de-asserting tready. Since we can't back pressure the MAC we have to
	     // abandon the current packet, discarding any data already sent down stream by sending an asserted error
	     // with a tlast when ever tready becomes asserted again. Meanwhile we start dropping arriving MAC
	     // data on the floor since there is nothing useful we can do with it currently.
	     if (axis_tready)
	       begin
		  // OK tready is asserted again so tlast is geting accepted this cycle along with an asserted error.
		  state <= EMPTY;
	       end else begin
		  // Keep error state asserted ready for downstream to accept
		  axis_tlast <= 1;
		  axis_tvalid <= 1;
		  axis_tuser <= {1'b1, EOF8}; // Force error in this packet.
	       end
	  end // case: ERROR1
	  

	endcase // case(state)
     end // else: !if(reset | clear)

endmodule
