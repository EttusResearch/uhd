//////////////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
// The ZYNQ FIFO - read FIFO and write to DDR:
//  - implements write state machine for AXI master on DDR
//  - provides input fifos from external fabric
//////////////////////////////////////////////////////////////////////////////////


//This implementation takes many states to do individual
//64 bit xfers from FIFO to the AXI write master.
//TODO: use axi 4/full with busts,
//in this case we should be able to directly connect the fifo
//to the write lines with much less state machinery.

module zf_stream_to_host
#(
    parameter PROT = 3'b010, //data, non-secure, unpriv
    parameter STRB = 4'b1111 //write all bytes
)
(
    input clk,
    input rst,
    input enb,

    //------------------------------------------------------------------
    //-- DDR write signals - master
    //------------------------------------------------------------------
    output [31:0] AXI_AWADDR,
    output [2:0] AXI_AWPROT,
    output AXI_AWVALID,
    input AXI_AWREADY,
    output [63:0] AXI_WDATA,
    output [3:0] AXI_WSTRB,
    output AXI_WVALID,
    input AXI_WREADY,
    input [1:0] AXI_BRESP,
    input AXI_BVALID,
    output AXI_BREADY,

    //------------------------------------------------------------------
    // FIFO streaming interfaces
    //------------------------------------------------------------------
    input [63:0] i_tdata,
    input i_tlast,
    input i_tvalid,
    output i_tready,

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
    localparam STATE_READ_LINE = 1;
    localparam STATE_WRITE_ADDR = 2;
    localparam STATE_WRITE_DATA = 3;
    localparam STATE_WRITE_B = 4;
    localparam STATE_DONE = 5;

    reg [31:0] base_addr;
    reg [63:0] line;
    reg last;

    reg [2:0] state;
    always @(posedge clk) begin
        if (rst) begin
            state <= STATE_WAIT_MEM;
            base_addr <= 0;
            line <= 0;
            last <= 0;
        end
        else if (enb) case (state)
        STATE_WAIT_MEM: begin
            if (mem_valid) begin
                state <= STATE_READ_LINE;
            end
            base_addr <= mem_addr;
        end

        STATE_READ_LINE: begin
            if (i_tvalid && i_tready) begin
                line <= i_tdata;
                last <= i_tlast;
                state <= STATE_WRITE_ADDR;
            end
        end

        STATE_WRITE_ADDR: begin
            if (AXI_AWVALID && AXI_AWREADY) begin
                state <= STATE_WRITE_DATA;
            end
        end

        STATE_WRITE_DATA: begin
            if (AXI_WVALID && AXI_WREADY) begin
                state <= STATE_WRITE_B;
            end
        end

        STATE_WRITE_B: begin
            if (AXI_BREADY && AXI_BVALID) begin//FIXME, slave may not assert valid
                if (last) state <= STATE_DONE;
                else state <= STATE_READ_LINE;
                base_addr <= base_addr + 32'h8;
            end
        end

        STATE_DONE: begin
            state <= STATE_WAIT_MEM;
        end

        default: state <= STATE_WAIT_MEM;

        endcase //state
    end

    assign i_tready = (state == STATE_READ_LINE);
    assign mem_ack = (state == STATE_DONE);

    //assign to master write
    assign AXI_AWVALID = (state == STATE_WRITE_ADDR);
    assign AXI_WVALID = (state == STATE_WRITE_DATA);
    assign AXI_AWADDR = base_addr;
    assign AXI_WDATA = {line[31:0], line[63:32]};

    assign AXI_WSTRB  = STRB;
    assign AXI_AWPROT = PROT;
    assign AXI_BREADY = (state == STATE_WRITE_B);

    assign debug[2:0] = state;
    assign debug[4] = mem_valid;
    assign debug[5] = mem_ack;
    assign debug[6] = AXI_AWVALID;
    assign debug[7] = AXI_AWREADY;
    assign debug[8] = AXI_WVALID;
    assign debug[9] = AXI_WREADY;
    assign debug[10] = AXI_BVALID;
    assign debug[11] = AXI_BREADY;

endmodule //zf_stream_to_host
