//
// Copyright 2013 Ettus Research LLC
//


module pcie_basic_regs #(
    parameter SIGNATURE = 32'h0,
    parameter CLK_FREQ  = 32'h0
) (
    input           clk,
    input           reset,

    input [63:0]    regi_tdata,
    input           regi_tvalid,
    output          regi_tready,
    output [63:0]   rego_tdata,
    output          rego_tvalid,
    input           rego_tready,
    
    input [31:0]    misc_status
);
    localparam PCIE_FPGA_SIG_VAL                = SIGNATURE;
    localparam PCIE_FPGA_COUNTER_FREQ           = CLK_FREQ;

    localparam PCIE_REG_ADDR_MASK               = 20'h001FF;

    localparam PCIE_FPGA_SIG_REG_ADDR           = 20'h00000;    //32-bit
    localparam PCIE_FPGA_COUNTER_LO_REG_ADDR    = 20'h00004;    //32-bit
    localparam PCIE_FPGA_COUNTER_HI_REG_ADDR    = 20'h00008;    //32-bit
    localparam PCIE_FPGA_COUNTER_FREQ_ADDR      = 20'h0000C;    //32-bit
    localparam PCIE_FPGA_SCRATCH0_ADDR          = 20'h00010;    //32-bit
    localparam PCIE_FPGA_SCRATCH1_ADDR          = 20'h00014;    //32-bit
    localparam PCIE_FPGA_MISC_STATUS_ADDR       = 20'h00020;    //32-bit
    localparam PCIE_FPGA_USR_SIG_REG_ADDR       = 20'h00030;    //128-bit

    wire            regi_wr, regi_rd;
    wire [19:0]     regi_addr, regi_addr_local;
    wire [31:0]     regi_payload;
    reg  [31:0]     rego_payload;

    ioport2_msg_decode regi_decoder (
        .message(regi_tdata), .wr_request(regi_wr), .rd_request(regi_rd),
        .address(regi_addr), .data(regi_payload)
    );

    ioport2_msg_encode  rego_encoder (
        .rd_response(1'b1), .data(rego_payload), .message(rego_tdata)
    );

    assign regi_tready = (regi_tvalid & regi_wr) | rego_tready;
    assign rego_tvalid = regi_tvalid & regi_rd;
    assign regi_addr_local = regi_addr & PCIE_REG_ADDR_MASK;

    //Counter counting bus_clk cycles
    reg [63:0]  bus_counter;
    always @(posedge clk) begin
        if (reset)  bus_counter <= 64'h0;
        else        bus_counter <= bus_counter + 1;
    end

    //Scratch registers
    reg [63:0]  scratch;
    always @(posedge clk) begin
        if (reset)  
            scratch <= 64'h0;
        else if (regi_tvalid & regi_tready & regi_wr)
            if (regi_addr_local == PCIE_FPGA_SCRATCH0_ADDR)
                scratch[31:0]  <= regi_payload;
            else if (regi_addr_local == PCIE_FPGA_SCRATCH1_ADDR)
                scratch[63:32] <= regi_payload;
    end
    
    //User signature register
    reg [127:0]  usr_signature;
    always @(posedge clk) begin
        if (reset)
            usr_signature <= 128'h0;
        else if (regi_tvalid & regi_tready & regi_wr)
            if (regi_addr_local == (PCIE_FPGA_USR_SIG_REG_ADDR + 20'h00000))
                usr_signature[31:0]  <= regi_payload;
            else if (regi_addr_local == (PCIE_FPGA_USR_SIG_REG_ADDR + 20'h00004))
                usr_signature[63:32]  <= regi_payload;
            else if (regi_addr_local == (PCIE_FPGA_USR_SIG_REG_ADDR + 20'h00008))
                usr_signature[95:64]  <= regi_payload;
            else if (regi_addr_local == (PCIE_FPGA_USR_SIG_REG_ADDR + 20'h0000C))
                usr_signature[127:96]  <= regi_payload;
    end

    always @(*) begin
        case (regi_addr_local)
            PCIE_FPGA_SIG_REG_ADDR:                 rego_payload = PCIE_FPGA_SIG_VAL;
            PCIE_FPGA_COUNTER_LO_REG_ADDR:          rego_payload = bus_counter[31:0];
            PCIE_FPGA_COUNTER_HI_REG_ADDR:          rego_payload = bus_counter[63:32];
            PCIE_FPGA_COUNTER_FREQ_ADDR:            rego_payload = PCIE_FPGA_COUNTER_FREQ;
            PCIE_FPGA_SCRATCH0_ADDR:                rego_payload = scratch[31:0];
            PCIE_FPGA_SCRATCH1_ADDR:                rego_payload = scratch[63:32];
            PCIE_FPGA_MISC_STATUS_ADDR:             rego_payload = misc_status;
            PCIE_FPGA_USR_SIG_REG_ADDR + 20'h00000: rego_payload = usr_signature[31:0];
            PCIE_FPGA_USR_SIG_REG_ADDR + 20'h00004: rego_payload = usr_signature[63:32];
            PCIE_FPGA_USR_SIG_REG_ADDR + 20'h00008: rego_payload = usr_signature[95:64];
            PCIE_FPGA_USR_SIG_REG_ADDR + 20'h0000C: rego_payload = usr_signature[127:96];
            default:                                rego_payload = 32'hFFFFFFFF;
        endcase
    end

endmodule
