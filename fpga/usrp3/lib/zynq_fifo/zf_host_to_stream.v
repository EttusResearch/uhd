//////////////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
// The ZYNQ FIFO - read DDR and write to FIFO:
//  - implements read state machine for AXI master on DDR
//  - provides output fifos to external fabric
//////////////////////////////////////////////////////////////////////////////////


module zf_host_to_stream
#(
    parameter PROT = 3'b010 //data, non-secure, unpriv
)
(
    input clk,
    input rst,
    input enb,

    //------------------------------------------------------------------
    //-- DDR read signals - master
    //------------------------------------------------------------------
    output [31:0] AXI_ARADDR,
    output [2:0] AXI_ARPROT,
    output AXI_ARVALID,
    input AXI_ARREADY,
    input [63:0] AXI_RDATA,
    input [1:0] AXI_RRESP,
    input AXI_RVALID,
    output AXI_RREADY,

    //------------------------------------------------------------------
    // FIFO streaming interfaces
    //------------------------------------------------------------------
    output [63:0] o_tdata,
    output o_tlast,
    output o_tvalid,
    input o_tready,

    //------------------------------------------------------------------
    // configuration interface
    //------------------------------------------------------------------
    input [31:0] mem_addr,
    input mem_valid,
    output mem_ack,

    output [31:0] debug
);

////////////////////////////////////////////////////////////////////////
///////////////////////////// Begin R T L //////////////////////////////
////////////////////////////////////////////////////////////////////////


    localparam STATE_WAIT_MEM = 0;
    localparam STATE_WRITE_ADDR = 1;
    localparam STATE_READ_DATA = 2;
    localparam STATE_WRITE_LINE = 3;
    localparam STATE_DONE = 4;

    reg [31:0] base_addr;
    reg [63:0] line;
    reg [15:0] line32_count;
    reg first_line;

    reg [2:0] state;
    always @(posedge clk) begin
        if (rst) begin
            state <= STATE_WAIT_MEM;
            base_addr <= 0;
            line <= 0;
            line32_count <= 0;
            first_line <= 1;
        end
        else if (enb) case (state)
        STATE_WAIT_MEM: begin
            if (mem_valid) begin
                state <= STATE_WRITE_ADDR;
            end
            base_addr <= mem_addr;
            first_line <= 1;
        end

        STATE_WRITE_ADDR: begin
            if (AXI_ARVALID && AXI_ARREADY) begin
                state <= STATE_READ_DATA;
            end
        end

        STATE_READ_DATA: begin
            if (AXI_RVALID && AXI_RREADY) begin
                line <= AXI_RDATA;
                state <= STATE_WRITE_LINE;
                if (first_line) begin
                    //round up to multiple of 64 minus one line
                    //Note! words32 are swapped here, inspect lower for length
                    line32_count <= AXI_RDATA[15:0] - 16'b1;
                    first_line <= 0;
                end
            end
        end

        STATE_WRITE_LINE: begin
            if (o_tvalid && o_tready) begin
                if (o_tlast) state <= STATE_DONE;
                else state <= STATE_WRITE_ADDR;
                base_addr <= base_addr + 32'h8;
                line32_count <= line32_count - 16'h2;
            end
        end

        STATE_DONE: begin
            state <= STATE_WAIT_MEM;
        end

        default: state <= STATE_WAIT_MEM;

        endcase //state
    end

    assign o_tdata = {line[31:0], line[63:32]};
    assign o_tlast = (line32_count[15:1] == 15'b0); //ignore low bit
    assign o_tvalid = (state == STATE_WRITE_LINE);
    assign mem_ack = (state == STATE_DONE);

    //the master read address always comes from the reg
    assign AXI_ARADDR = base_addr;
    assign AXI_ARVALID = (state == STATE_WRITE_ADDR);
    assign AXI_RREADY = (state == STATE_READ_DATA);
    assign AXI_ARPROT = PROT;

    assign debug[2:0] = state;
    assign debug[3] = first_line;
    assign debug[4] = mem_valid;
    assign debug[5] = mem_ack;
    assign debug[6] = AXI_ARVALID;
    assign debug[7] = AXI_ARREADY;
    assign debug[8] = AXI_RVALID;
    assign debug[9] = AXI_RREADY;

endmodule //zf_host_to_stream
