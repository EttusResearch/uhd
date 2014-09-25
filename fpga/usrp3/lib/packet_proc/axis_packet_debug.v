//
// Copyright 2014 Ettus Research LLC
//

module axis_packet_debug (
   input          clk,
   input          reset,
   input          clear,

   //Packet In
   input [63:0]   tdata,
   input          tlast,
   input          tvalid,
   input          tready,

   //Per packet info
   output reg           pkt_strobe,
   output reg [15:0]    length,
   output reg [63:0]    checksum,
   
   //Statistics
   output reg [31:0]    pkt_count
);

   localparam ST_HEADER = 1'b0;
   localparam ST_DATA   = 1'b1;

   //Packet state logic
   reg   pkt_state;
   always @(posedge clk) begin
      if (reset) begin
         pkt_state <= ST_HEADER;
      end else if (tvalid & tready) begin
         pkt_state <= tlast ? ST_HEADER : ST_DATA;
      end
   end

   //Trigger logic
   always @(posedge clk)
      if (reset)
         pkt_strobe <= 1'b0;
      else
         pkt_strobe <= tvalid & tready & tlast;

   //Length capture
   always @(posedge clk)
      if (reset || pkt_state == ST_HEADER)
         length <= tlast ? 16'd8 : 16'd0;
      else
         if (tvalid & tready)
            length <= length + 16'd8;

   //Checksum capture
   always @(posedge clk)
      if (reset || pkt_state == ST_HEADER)
         checksum <= 64'd0;
      else
         if (tvalid & tready)
            checksum <= checksum ^ tdata;

   //Counts
   always @(posedge clk)
      if (reset | clear) begin
         pkt_count <= 32'd0;
      end else begin
         if (tvalid & tready & tlast) begin
            pkt_count <= pkt_count + 32'd1;
         end
      end

endmodule // cvita_packet_debug
