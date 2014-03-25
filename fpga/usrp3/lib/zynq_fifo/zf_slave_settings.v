//////////////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
// The ZYNQ FIFO slave settings:
//  - implements write state machine for 32-bit addressable AXI slave
//  - provides settings for state of per-stream fifo pointers
//  - implements configuration of FIFO's physical DDR addresses
//////////////////////////////////////////////////////////////////////////////////


module zf_slave_settings
#(
    parameter CONFIG_BASE = 32'h40000000
)
(
    input clk,
    input rst,

    //------------------------------------------------------------------
    //-- control write signals - slave
    //------------------------------------------------------------------
    input [31:0] AXI_AWADDR,
    input AXI_AWVALID,
    output AXI_AWREADY,
    input [31:0] AXI_WDATA,
    input [3:0] AXI_WSTRB,
    input AXI_WVALID,
    output AXI_WREADY,
    output [1:0] AXI_BRESP,
    output AXI_BVALID,
    input AXI_BREADY,

    //------------------------------------------------------------------
    // settings interface
    //------------------------------------------------------------------
    output reg [31:0] addr,
    output reg [31:0] data,
    output strobe,

    output [31:0] debug
);

////////////////////////////////////////////////////////////////////////
///////////////////////////// Begin R T L //////////////////////////////
////////////////////////////////////////////////////////////////////////

    //------------------------------------------------------------------
    // Control write state machine responds to AXI control writes
    // Used for setting the state of the various FIFOs
    //------------------------------------------------------------------
    localparam STATE_ADDR = 0;
    localparam STATE_DATA = 1;
    localparam STATE_WRITE = 2;

    reg [1:0] state;

    always @(posedge clk) begin
        if (rst) begin
            state <= STATE_ADDR;
            addr <= 0;
            data <= 0;
        end
        else case (state)

        STATE_ADDR: begin
            if (AXI_AWVALID && AXI_AWREADY) begin
                addr <= (AXI_AWADDR - CONFIG_BASE);
                state <= STATE_DATA;
            end
        end

        STATE_DATA: begin
            if (AXI_WVALID && AXI_WREADY) begin
                data <= AXI_WDATA;
                state <= STATE_WRITE;
            end
        end

        STATE_WRITE: begin
            state <= STATE_ADDR;
        end

        default: state <= STATE_ADDR;

        endcase //state
    end

    assign strobe = (state == STATE_WRITE);

    //assign to slave write
    assign AXI_AWREADY = (state == STATE_ADDR);
    assign AXI_WREADY = (state == STATE_DATA);
    assign AXI_BRESP = 0;
    assign AXI_BVALID = AXI_BREADY; //FIXME - we can choose not to assert valid

endmodule //zf_slave_settings
