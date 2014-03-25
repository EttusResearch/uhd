//
// Copyright 2013 Ettus Research LLC
//


module pcie_pkt_route_specifier #(
    parameter BASE_ADDR = 20'h0,
    parameter ADDR_MASK = 20'hFFF00,
    parameter SID_WIDTH = 8,
    parameter DST_WIDTH = 4
) (
    input           clk,
    input           reset,

    input [63:0]    regi_tdata,
    input           regi_tvalid,
    output          regi_tready,

    input  [SID_WIDTH-1:0]  local_sid,
    output [DST_WIDTH-1:0]  fifo_dst
);
    // Routing table
    reg [DST_WIDTH-1:0] routing_table[0:(1<<SID_WIDTH)-1];
    assign fifo_dst = routing_table[local_sid];
    
    wire        reg_wr;
    wire [19:0] reg_addr;
    wire [31:0] reg_data;
    
    // Routing table configuration
    ioport2_msg_decode config_message_decoder (
        .message(regi_tdata), .wr_request(reg_wr), .rd_request(reg_rd), .address(reg_addr), .data(reg_data)
    );

    always @(posedge clk) begin
      if (regi_tvalid && regi_tready && reg_wr && ((reg_addr & ADDR_MASK) == BASE_ADDR)) begin
        routing_table[reg_data[SID_WIDTH+15:16]] <= reg_data[DST_WIDTH-1:0];
      end
    end
    
    assign regi_tready = 1;
    
endmodule
