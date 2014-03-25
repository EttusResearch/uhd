//////////////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
// The ZYNQ FIFO slave readback:
//  - implements read state machine for 32-bit addressable AXI slave
//  - provides readback for state of per-stream fifo pointers
//////////////////////////////////////////////////////////////////////////////////


module zf_slave_readback
#(
    parameter CONFIG_BASE = 32'h40000000
)
(
    input clk,
    input rst,

    //------------------------------------------------------------------
    //-- control read signals - slave
    //------------------------------------------------------------------
    input [31:0] AXI_ARADDR,
    input AXI_ARVALID,
    output AXI_ARREADY,
    output [31:0] AXI_RDATA,
    output [1:0] AXI_RRESP,
    output AXI_RVALID,
    input AXI_RREADY,

    //------------------------------------------------------------------
    // readback interface
    //------------------------------------------------------------------
    output reg [31:0] addr,
    input [31:0] data,
    output strobe,

    output [31:0] debug
);

////////////////////////////////////////////////////////////////////////
///////////////////////////// Begin R T L //////////////////////////////
////////////////////////////////////////////////////////////////////////

    //------------------------------------------------------------------
    // Control read state machine responds to AXI control reads
    // Used for reading back the state of the various FIFOs
    //------------------------------------------------------------------
    localparam STATE_ADDR = 0;
    localparam STATE_READ = 1;
    localparam STATE_DATA = 2;

    reg [3:0] state;

    always @(posedge clk) begin
        if (rst) begin
            state <= STATE_ADDR;
            addr <= 0;
        end
        else case (state)

        STATE_ADDR: begin
            if (AXI_ARVALID && AXI_ARREADY) begin
                state <= STATE_READ;
                addr <= (AXI_ARADDR - CONFIG_BASE);
            end
        end

        STATE_READ: begin
            state <= STATE_DATA;
        end

        STATE_DATA: begin
            if (AXI_RVALID && AXI_RREADY) begin
                state <= STATE_ADDR;
            end
        end

        default: state <= STATE_ADDR;

        endcase //state
    end

    assign strobe = AXI_RVALID && AXI_RREADY;

    //readback data
    assign AXI_RDATA = data;
    //only acking address reads from the wait state
    assign AXI_ARREADY = (state == STATE_ADDR);
    //when to release outputs from the slave
    assign AXI_RVALID = (state == STATE_DATA);
    assign AXI_RRESP = 0;

endmodule //zf_slave_readback
