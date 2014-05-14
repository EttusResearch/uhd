//
// Copyright 2013 Ettus Research LLC
//


`define BIT_WIDTH(N) (\
                 N <= 2    ? 1 : \
                 N <= 4    ? 2 : \
                 N <= 8    ? 3 : \
                 N <= 16   ? 4 : \
                 N <= 32   ? 5 : \
                 N <= 64   ? 6 : \
                 N <= 128  ? 7 : \
                 N <= 256  ? 8 : \
                 N <= 512  ? 9 : \
                 10)
`define GET_REG_OFFSET(reg_addr, chan_idx) (((chan_idx * (1<<DMA_REG_GRP_W)) + reg_addr) + REG_BASE_ADDR)
`define EXTRACT_CHAN_NUM(reg_addr) regi_addr[`BIT_WIDTH(NUM_STREAMS)+DMA_REG_GRP_W-1:DMA_REG_GRP_W]

module pcie_dma_ctrl #(
    parameter NUM_STREAMS   = 4,
    parameter FRAME_SIZE_W  = 16,
    parameter REG_BASE_ADDR = 20'h00000,
    parameter ENABLE_ROUTER = 0,
    parameter ROUTER_SID_W  = 8,
    parameter ROUTER_DST_W  = 2    
) (
    input           clk,
    input           reset,

    input [63:0]    regi_tdata,
    input           regi_tvalid,
    output          regi_tready,
    output [63:0]   rego_tdata,
    output          rego_tvalid,
    input           rego_tready,
    
    output reg [NUM_STREAMS-1:0]            set_enabled,
    output reg [NUM_STREAMS-1:0]            set_clear,
    output [(NUM_STREAMS*FRAME_SIZE_W)-1:0] set_frame_size,
    output [(NUM_STREAMS*3)-1:0]            swap_lanes,
    
    input  [NUM_STREAMS-1:0]                packet_stb,
    input  [NUM_STREAMS-1:0]                sample_stb,
    input  [NUM_STREAMS-1:0]                stream_busy,
    input  [NUM_STREAMS-1:0]                stream_err,

    input  [ROUTER_SID_W-1:0]               rtr_sid,
    output [ROUTER_DST_W-1:0]               rtr_dst    
);
    
    localparam DMA_REG_GRP_W        = 4;
    localparam DMA_CTRL_STATUS_REG  = 4'h0; //[RW] R: Stream Status, W: Stream Control
    localparam DMA_FSIZE_REG        = 4'h4; //[RW] R: Frame Size, W: Frame Size
    localparam DMA_SAMP_CNT_REG     = 4'h8; //[RW] R: Sample Count, W: Reset Count to 0
    localparam DMA_PKT_CNT_REG      = 4'hC; //[RW] R: Packet Count, W: Reset Count to 0
    
    localparam DEFAULT_FSIZE        = 32;

    //NOTE: Although this module supports these, the 8 and 16 bit modes will be disabled for efficiency
    localparam DMA_CTRL_BUF_SIZE_8  = 2'b00;    // 8-bit wide SW buffer
    localparam DMA_CTRL_BUF_SIZE_16 = 2'b01;    //16-bit wide SW buffer
    localparam DMA_CTRL_BUF_SIZE_32 = 2'b10;    //32-bit wide SW buffer
    localparam DMA_CTRL_BUF_SIZE_64 = 2'b11;    //64-bit wide SW buffer

    wire            regi_wr, regi_rd;
    wire [19:0]     regi_addr;
    wire [31:0]     regi_payload;
    wire [31:0]     rego_payload;

    ioport2_msg_decode regi_decoder (
        .message(regi_tdata), .wr_request(regi_wr), .rd_request(regi_rd),
        .address(regi_addr), .data(regi_payload)
    );

    ioport2_msg_encode  rego_encoder (
        .rd_response(1'b1), .data(rego_payload), .message(rego_tdata)
    );

    reg [31:0]              pkt_count_mem[0:NUM_STREAMS-1];
    reg [31:0]              samp_count_mem[0:NUM_STREAMS-1];
    reg [FRAME_SIZE_W-1:0]  frame_size_mem[0:NUM_STREAMS-1];
    reg [NUM_STREAMS-1:0]   sw_buf_width_mem;

    genvar i;
    generate
        for (i=0; i<NUM_STREAMS; i=i+1) begin: dma_ctrl_logic_generator
            //Memory -> output translations
            assign set_frame_size[(FRAME_SIZE_W*(i+1))-1:(FRAME_SIZE_W*i)] = frame_size_mem[i];
            assign swap_lanes[(3*(i+1))-1:(3*i)] = { ~(sw_buf_width_mem[i]), 2'b00 }; //Optimized for only 2 modes
        
            //Setting registers
            always @(posedge clk) begin
                if (reset) begin
                    frame_size_mem[i] <= DEFAULT_FSIZE;
                    set_clear[i] <= 0;
                    set_enabled[i] <= 0;
                    sw_buf_width_mem[i] <= 1;
                end else if (regi_tready & regi_tvalid & regi_wr) begin
                    if (regi_addr == `GET_REG_OFFSET(DMA_CTRL_STATUS_REG, i)) begin
                        set_clear[i]        <= regi_payload[0];                 //DMA_CTRL_STATUS_REG[0] == Clear DMA queues
                        set_enabled[i]      <= regi_payload[1];                 //DMA_CTRL_STATUS_REG[1] == Enable DMA channel
                        sw_buf_width_mem[i] <= regi_payload[4];                 //DMA_CTRL_STATUS_REG[5:4] == SW Buffer Size (See note above)
                    end else if (regi_addr == `GET_REG_OFFSET(DMA_FSIZE_REG, i)) begin
                        frame_size_mem[i] <= regi_payload[FRAME_SIZE_W-1:0];    //DMA_FSIZE_REG[14:0] == DMA Frame size
                    end
                end else begin
                    set_clear[i] <= 0;                                          //set_clear should be "self-clearing"
                end
            end

            //Packet counter
            always @(posedge clk) begin
                if (reset | (regi_tvalid && regi_wr && (regi_addr == `GET_REG_OFFSET(DMA_PKT_CNT_REG, i)))) begin
                    pkt_count_mem[i] <= 0;
                end else if (packet_stb[i]) begin
                    pkt_count_mem[i] <= pkt_count_mem[i] + 1;
                end
            end

            //Sample counter
            always @(posedge clk) begin
                if (reset | (regi_tvalid && regi_wr && (regi_addr == `GET_REG_OFFSET(DMA_SAMP_CNT_REG, i)))) begin
                    samp_count_mem[i] <= 0;
                end else if (sample_stb[i]) begin
                    samp_count_mem[i] <= samp_count_mem[i] + 1;
                end
            end
        end
    endgenerate
    
    //Readback
    assign rego_payload =
        (regi_addr[DMA_REG_GRP_W-1:0] == DMA_PKT_CNT_REG)     ?      pkt_count_mem[`EXTRACT_CHAN_NUM(regi_addr)]  : (
        (regi_addr[DMA_REG_GRP_W-1:0] == DMA_SAMP_CNT_REG)    ?     samp_count_mem[`EXTRACT_CHAN_NUM(regi_addr)]  : (
        (regi_addr[DMA_REG_GRP_W-1:0] == DMA_FSIZE_REG)       ?     frame_size_mem[`EXTRACT_CHAN_NUM(regi_addr)]  : (
        (regi_addr[DMA_REG_GRP_W-1:0] == DMA_CTRL_STATUS_REG) ? {30'h0, stream_busy[`EXTRACT_CHAN_NUM(regi_addr)], stream_err[`EXTRACT_CHAN_NUM(regi_addr)]} : (
        32'hFFFFFFFF))));

    assign rego_tvalid = regi_tvalid && regi_rd;
    assign regi_tready = rego_tready || (regi_tvalid && regi_wr);
    
    //Optional router
    generate if (ENABLE_ROUTER == 1) begin
        pcie_pkt_route_specifier #(
            .BASE_ADDR((1<<ROUTER_SID_W) + REG_BASE_ADDR), .ADDR_MASK(20'hFFFFF^((1<<ROUTER_SID_W)-1)),
            .SID_WIDTH(ROUTER_SID_W), .DST_WIDTH(ROUTER_DST_W)
        ) route_specifier (
            .clk(clk), .reset(reset),
            .regi_tdata(regi_tdata), .regi_tvalid(regi_tvalid), .regi_tready(),
            .local_sid(rtr_sid), .fifo_dst(rtr_dst)
        );
    end endgenerate
    
endmodule

`undef EXTRACT_CHAN_NUM
`undef GET_REG_OFFSET
`undef BIT_WIDTH
