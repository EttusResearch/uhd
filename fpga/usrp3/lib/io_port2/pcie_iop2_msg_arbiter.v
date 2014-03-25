//
// Copyright 2013 Ettus Research LLC
//


module pcie_iop2_msg_arbiter #(
    parameter E0_ADDR = 20'h0,
    parameter E0_MASK = 20'h0,
    parameter E1_ADDR = 20'h0,
    parameter E1_MASK = 20'h0,
    parameter E2_ADDR = 20'h0,
    parameter E2_MASK = 20'h0,
    parameter E3_ADDR = 20'h0,
    parameter E3_MASK = 20'h0
) (
    //Clocks and resets
    input           clk,
    input           reset,

    input [63:0]    regi_tdata,
    input           regi_tvalid,
    output          regi_tready,
    output [63:0]   rego_tdata,
    output          rego_tvalid,    
    input           rego_tready,

    output [63:0]   e0_regi_tdata,
    output          e0_regi_tvalid,
    input           e0_regi_tready,
    input [63:0]    e0_rego_tdata,
    input           e0_rego_tvalid,    
    output          e0_rego_tready,
    
    output [63:0]   e1_regi_tdata,
    output          e1_regi_tvalid,
    input           e1_regi_tready,
    input [63:0]    e1_rego_tdata,
    input           e1_rego_tvalid,    
    output          e1_rego_tready,

    output [63:0]   e2_regi_tdata,
    output          e2_regi_tvalid,
    input           e2_regi_tready,
    input [63:0]    e2_rego_tdata,
    input           e2_rego_tvalid,    
    output          e2_rego_tready,

    output [63:0]   e3_regi_tdata,
    output          e3_regi_tvalid,
    input           e3_regi_tready,
    input [63:0]    e3_rego_tdata,
    input           e3_rego_tvalid,    
    output          e3_rego_tready
);

    //*******************************************************************************
    // PCIe output message arbiter
    //
    axi_mux4 #(.PRIO(0), .WIDTH(64), .BUFFER(0)) rego_arbiter_mux (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i0_tdata(e0_rego_tdata), .i0_tlast(e0_rego_tvalid), .i0_tvalid(e0_rego_tvalid), .i0_tready(e0_rego_tready),
        .i1_tdata(e1_rego_tdata), .i1_tlast(e1_rego_tvalid), .i1_tvalid(e1_rego_tvalid), .i1_tready(e1_rego_tready),
        .i2_tdata(e2_rego_tdata), .i2_tlast(e2_rego_tvalid), .i2_tvalid(e2_rego_tvalid), .i2_tready(e2_rego_tready),
        .i3_tdata(e3_rego_tdata), .i3_tlast(e3_rego_tvalid), .i3_tvalid(e3_rego_tvalid), .i3_tready(e3_rego_tready),
        .o_tdata(rego_tdata), .o_tlast(), .o_tvalid(rego_tvalid), .o_tready(rego_tready)
    );
    //
    //*******************************************************************************

    //*******************************************************************************
    // PCIe input message arbiter
    //
    wire [63:0]     regi_msg;
    wire            regi_rc;
    wire [19:0]     regi_addr;
    wire            e0_rego_rd, e1_rego_rd, e2_rego_rd, e3_rego_rd;

    ioport2_msg_decode e0_rego_decoder (.message(e0_rego_tdata), .rd_request(e0_rego_rd));
    ioport2_msg_decode e1_rego_decoder (.message(e1_rego_tdata), .rd_request(e1_rego_rd));
    ioport2_msg_decode e2_rego_decoder (.message(e2_rego_tdata), .rd_request(e2_rego_rd));
    ioport2_msg_decode e3_rego_decoder (.message(e3_rego_tdata), .rd_request(e3_rego_rd));

    localparam DEST_E0   = 2'd0;
    localparam DEST_E1   = 2'd1;
    localparam DEST_E2   = 2'd2;
    localparam DEST_E3   = 2'd3;

    reg  [1:0]  regi_resp_dest;
    wire [1:0]  regi_req_dest, regi_dest;
    
    assign regi_req_dest = 
        ((regi_addr & E0_MASK) == E0_ADDR) ? DEST_E0 : (
        ((regi_addr & E1_MASK) == E1_ADDR) ? DEST_E1 : (
        ((regi_addr & E2_MASK) == E2_ADDR) ? DEST_E2 : (
        ((regi_addr & E3_MASK) == E3_ADDR) ? DEST_E3 : (
        DEST_E0))));
    
    //A response must be routed to the port with the last read request 
    always @(posedge clk) begin
        if (reset)
            regi_resp_dest <= DEST_E0;  //Default 0
        else if (e0_rego_tvalid & e0_rego_tready & e0_rego_rd)
            regi_resp_dest <= DEST_E0;
        else if (e1_rego_tvalid & e1_rego_tready & e1_rego_rd)
            regi_resp_dest <= DEST_E1;
        else if (e2_rego_tvalid & e2_rego_tready & e2_rego_rd)
            regi_resp_dest <= DEST_E2;
        else if (e3_rego_tvalid & e3_rego_tready & e3_rego_rd)
            regi_resp_dest <= DEST_E3;
    end

    ioport2_msg_decode regi_decoder (
        .message(regi_msg), .rd_response(regi_rc), .address(regi_addr));

    //If request, get destination from msg. 
    //If response, get destination from last read location. 
    assign regi_dest = regi_rc ? regi_resp_dest : regi_req_dest;

    axi_demux4 #(.ACTIVE_CHAN(4'b1111), .WIDTH(64), .BUFFER(0)) regi_arbiter_demux (
        .clk(clk), .reset(reset), .clear(1'b0),
        .header(regi_msg), .dest(regi_dest),
        .i_tdata(regi_tdata), .i_tlast(regi_tvalid), .i_tvalid(regi_tvalid), .i_tready(regi_tready),
        .o0_tdata(e0_regi_tdata), .o0_tlast(), .o0_tvalid(e0_regi_tvalid), .o0_tready(e0_regi_tready),
        .o1_tdata(e1_regi_tdata), .o1_tlast(), .o1_tvalid(e1_regi_tvalid), .o1_tready(e1_regi_tready),
        .o2_tdata(e2_regi_tdata), .o2_tlast(), .o2_tvalid(e2_regi_tvalid), .o2_tready(e2_regi_tready),
        .o3_tdata(e3_regi_tdata), .o3_tlast(), .o3_tvalid(e3_regi_tvalid), .o3_tready(e3_regi_tready)
    );
    //
    //*******************************************************************************

endmodule
