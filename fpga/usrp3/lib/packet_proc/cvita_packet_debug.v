//
// Copyright 2014 Ettus Research LLC
//

module cvita_packet_debug (
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
   output reg [63:0]    header,
   output reg [63:0]    timestamp,
   output reg [15:0]    actual_length,
   output reg [63:0]    checksum,
   
   //Statistics
   output reg [31:0]    pkt_count,
   output reg [31:0]    ctrl_pkt_count
);

   localparam ST_HEADER = 2'd0;
   localparam ST_TIME   = 2'd1;
   localparam ST_DATA   = 2'd2;

   //Packet state logic
   reg [1:0]   pkt_state;
   always @(posedge clk) begin
      if (reset) begin
         pkt_state <= ST_HEADER;
      end else if (tvalid & tready) begin
         case(pkt_state)
            ST_HEADER: begin
               if (!tlast) 
                  pkt_state <= (tdata[61]) ? ST_TIME : ST_DATA;
            end
            ST_TIME: begin
               pkt_state <= (tlast) ? ST_HEADER : ST_DATA;
            end
            ST_DATA: begin
               pkt_state <= (tlast) ? ST_HEADER : ST_DATA;
            end
            default: pkt_state <= ST_HEADER;
         endcase
      end
   end

   //Trigger logic
   always @(posedge clk)
      if (reset)
         pkt_strobe <= 1'b0;
      else
         pkt_strobe <= tvalid & tready & tlast;

   //Header capture
   always @(posedge clk)
      if (reset)
         header <= 64'd0;
      else if (pkt_state == ST_HEADER)
         if (tvalid & tready)
            header <= tdata;

   //Timestamp capture
   always @(posedge clk)
      if (reset)
         timestamp <= 64'd0;
      else if (pkt_state == ST_TIME)
         if (tvalid & tready)
            timestamp <= tdata;

   //Length capture
   always @(posedge clk)
      if (reset || pkt_state == ST_HEADER)
         actual_length <= (tvalid & tready & tlast) ? 16'd8 : 16'd0;
      else
         if (tvalid & tready)
            actual_length <= actual_length + 16'd8;

   //Checksum capture
   always @(posedge clk)
      if (reset || pkt_state == ST_HEADER)
         checksum <= 64'd0;
      else if (pkt_state == ST_DATA)
         if (tvalid & tready)
            checksum <= checksum ^ tdata;

   //Counts
   always @(posedge clk)
      if (reset | clear) begin
         pkt_count      <= 32'd0;
         ctrl_pkt_count <= 32'd0;
      end else begin
         if (tvalid & tready & tlast) begin
            pkt_count <= pkt_count + 32'd1;
            if (pkt_state == ST_HEADER && tdata[63])
               ctrl_pkt_count <= ctrl_pkt_count + 32'd1;
            else if (header[63]) 
               ctrl_pkt_count <= ctrl_pkt_count + 32'd1;
         end
      end

endmodule // cvita_packet_debug
