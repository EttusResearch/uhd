//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This module implements a highly customized content-addressable memory (CAM)
// that enables forwarding decisions to be made on a 16 bit field from a stream ID (SID) field.
// The forwarding is generic in the sense that a SID's host destination can map to any endpoint / crossbar port.
//
// The 16 bits are allocated by convention as 8 bits of Network address (addresses USRP's / AXI crossbars) and
// 8 bits of Host address (addresses endpoints / crossbar ports in a USRP).
//
// By definition if the destination field in the SID addresses a different
// USRP / crossbar than this one then we don't care about the Host field, only the Network field.
// We only look at the Host field when the Network field addresses us.
// Thus we need a CAM of 256+256 entries with Log2(N) bits, where N is the number of
// slave(output) ports on the crossbar switch.
//
// SID format:
//
// |---------|---------|---------|---------|
// |   SRC   |   SRC   |   DST   |   DST   |
// | NETWORK |   HOST  | NETWORK |   HOST  |
// |---------|---------|---------|---------|
//      8         8         8         8

module axi_forwarding_cam
  #(
    parameter BASE = 0,      // BASE address for setting registers in this block. (512 addrs used)
    parameter WIDTH=64,      // Bit width of FIFO word.
    parameter NUM_OUTPUTS=2  // Number of outputs (destinations) in crossbar.
    )
    (
     input 			  clk,
     input 			  reset,
     input 			  clear,
     // Monitored FIFO signals
     input [WIDTH-1:0] 		  o_tdata,
     input 			  o_tvalid,
     input 			  o_tready,
     input 			  o_tlast,
     input 			  pkt_present,
     // Configuration
     input [7:0] 		  local_addr,
     // Setting Bus
     input 			  set_stb,
     input [15:0] 		  set_addr,
     input [31:0] 		  set_data,

     output reg [NUM_OUTPUTS-1:0] forward_valid,
     input [NUM_OUTPUTS-1:0] 	  forward_ack,

     input                        rb_rd_stb,
     input [$clog2(NUM_OUTPUTS)-1:0] rb_addr,
     output [31:0]                rb_data
     );


   localparam WAIT_SOF = 0;
   localparam WAIT_EOF = 1;
   reg 				  state;

   localparam IDLE = 0;
   localparam FORWARD = 1;
   localparam WAIT = 2;

   reg 	[1:0]			  demux_state;

   reg [15:0] 			  dst;
   reg 				  dst_valid, dst_valid_reg;
   wire 			  local_dst;
   wire [8:0] 			  read_addr;

   //
   // Monitor packets leaving FIFO
   //
   always @(posedge clk)
     if (reset | clear) begin
        state <= WAIT_SOF;
     end else
       case(state)
	 //
	 // After RESET or the EOF of previous packet, the first cycle with
	 // output valid asserted is the SOF and presents the Header word.
	 // The cycle following the concurrent presentation of asserted output
	 // valid and output ready presents the word following the header.
	 //
         WAIT_SOF:
           if (o_tvalid && o_tready) begin
              state <= WAIT_EOF;
           end else begin
              state <= WAIT_SOF;
           end
	 //
	 // EOF is signalled by o_tlast asserted whilst output valid and ready asserted.
	 //
         WAIT_EOF:
           if (o_tlast && o_tvalid && o_tready) begin
              state <= WAIT_SOF;
           end else begin
              state <= WAIT_EOF;
           end
       endcase // case(in_state)

   //
   // Extract Destination fields(s) from SID
   //
   always @(posedge clk)
     if (reset | clear) begin
	dst <= 0;
	dst_valid <= 0;
	dst_valid_reg <= 0;
     end else if (o_tvalid && (state == WAIT_SOF) && pkt_present) begin
	// SID will remain valid until o_tready is asserted as this will cause a state transition.
	dst <= o_tdata[15:0];
	dst_valid <= 1;
	dst_valid_reg <= dst_valid;
     end else begin
	dst_valid <= 0;
	dst_valid_reg <= dst_valid;
     end

   //
   // Is Network field in DST our local address?
   //
   assign local_dst = (dst[15:8] == local_addr) && dst_valid;


   //
   // Mux address to RAM so that it searches CAM for Network field or Host field.
   // Network addresses are stored in the lower 256 locations, host addresses the upper 256.
   //
   assign read_addr = {local_dst,(local_dst ? dst[7:0] : dst[15:8])};

   //
   // Implement CAM as block RAM here, 512xCeil(Log2(NUM_OUTPUTS))
   //
   //synthesis attribute ram_style of mem is block
   reg [$clog2(NUM_OUTPUTS)-1 : 0] mem [0:511];

   // Initialize the CAM's local address forwarding decisions with sensible defaults by
   // assuming dst[7:4] = crossbar port, dst[3:0] = block port. Setup a one-to-one mapping
   // for crossbar ports and always map same crossbar port regardless of block port.
   // i.e.
   //   dst 8'h00 => forward to crossbar port 0
   //   dst 8'h01 => forward to crossbar port 0
   //   dst 8'h10 => forward to crossbar port 1
   // etc.
   integer xbar_port;
   integer block_port;
   initial begin
     for (xbar_port = 0; xbar_port < NUM_OUTPUTS; xbar_port = xbar_port + 1) begin
       for (block_port = 0; block_port < 16; block_port = block_port + 1) begin
         mem[256+(xbar_port << 4)+block_port] = xbar_port;
       end
     end
   end

   reg [8:0] 			   read_addr_reg;
   wire 			   write;
   wire [$clog2(NUM_OUTPUTS)-1:0] 	   read_data;

   assign write = (set_addr[15:9] == (BASE >>9)) && set_stb; // Addr decode.

   always @(posedge clk)
     begin
	read_addr_reg <= read_addr;

	if (write) begin
	   mem[set_addr[8:0]] <= set_data[$clog2(NUM_OUTPUTS)-1:0];
	end

     end

   assign read_data = mem[read_addr_reg];


   //
   // State machine to manage forwarding flags.
   //
    always @(posedge clk)
     if (reset | clear) begin
        forward_valid <= {NUM_OUTPUTS{1'b0}};
        demux_state <= IDLE;
     end else
       case(demux_state)

	 // Wait for Valid DST which indicates a new packet lookup in the CAM.
	 IDLE: begin
	    if (dst_valid_reg == 1) begin
	       forward_valid <= 1'b1 << read_data;
	       demux_state <= FORWARD;
	    end
	 end
	 // When Slave/Output thats forwarding ACK's the forward flag, clear request and wait for packet to be transfered
	 FORWARD: begin
	    if ((forward_ack & forward_valid) != 0) begin
	       forward_valid <= {NUM_OUTPUTS{1'b0}};
	       demux_state <= WAIT;
	    end
	 end
	 // When packet transfered go back to idle.
	 WAIT: begin
	    if (forward_ack == 0)
	      demux_state <= IDLE;
	 end

       endcase // case (demux_state)

  //
  // Compile forwarding statistics
  // (This uses a lot of registers!)
  //
  genvar m;
  reg [31:0] statistics [0:NUM_OUTPUTS-1];

  generate
    for (m = 0; m < NUM_OUTPUTS; m = m + 1) begin: generate_stats
      always @(posedge clk) begin
        if (reset | clear) begin
          statistics[m] <= 0;
        end else if (forward_ack[m] & forward_valid[m]) begin
            statistics[m] <= statistics[m] + 1;
        end
      end
    end
  endgenerate

  assign rb_data = statistics[rb_addr];

endmodule
