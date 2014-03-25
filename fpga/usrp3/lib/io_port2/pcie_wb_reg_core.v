//
// Copyright 2013 Ettus Research LLC
//


module pcie_wb_reg_core #(
    parameter WB_ADDRW  = 16,
    parameter WB_DATAW  = 32
)(
    input                   clk, 
    input                   rst,

    input                   wb_stb_i,
    input                   wb_we_i,
    input  [WB_ADDRW-1:0]   wb_adr_i,
    input  [WB_DATAW-1:0]   wb_dat_i,
    output                  wb_ack_o,
    output [WB_DATAW-1:0]   wb_dat_o,

    input  [63:0]           msgi_tdata,
    input                   msgi_tvalid,
    output                  msgi_tready,

    output [63:0]           msgo_tdata,
    output                  msgo_tvalid,
    input                   msgo_tready,

    output [31:0]           debug 
);
    // Parameters
    localparam PCIE_REGPORT_ADDR_MASK   = 20'h0FFFF;
    localparam PCIE_REGPORT_DATA_ADDR   = 20'h70000;
    localparam PCIE_REGPORT_READ_ADDR   = 20'h60000;
    localparam PCIE_REGPORT_STATUS_ADDR = 20'h60000;

    //------------------------------------------
    // WB AXI interface
    //
    wire  [63:0]    wb_msgi_tdata, wb_msgo_tdata;
    wire            wb_msgi_tvalid, wb_msgi_tready, wb_msgo_tvalid, wb_msgo_tready;
    wire            wb_monitor_active, wb_req_pending, wb_resp_pending;

    pcie_axi_wb_conv #( .WB_ADDRW(WB_ADDRW), .WB_DATAW(WB_DATAW) ) axi_wb_translator (
        .clk(clk), .rst(rst),
        .wb_stb_i(wb_stb_i), .wb_we_i(wb_we_i), .wb_adr_i(wb_adr_i),
        .wb_dat_i(wb_dat_i), .wb_ack_o(wb_ack_o), .wb_dat_o(wb_dat_o),
        .msgi_tdata(wb_msgo_tdata), .msgi_tvalid(wb_msgo_tvalid), .msgi_tready(wb_msgo_tready),
        .msgo_tdata(wb_msgi_tdata), .msgo_tvalid(wb_msgi_tvalid), .msgo_tready(wb_msgi_tready),
        .wb_monitor_active(wb_monitor_active), .wb_req_pending(wb_req_pending), .wb_resp_pending(wb_resp_pending)
    );
    //------------------------------------------

    //------------------------------------------
    // PCIe In -> WB Out
    //
    wire            pcie_in_wr, pcie_in_rd, wb_out_wr, wb_out_rd; 
    wire            pcie2wb_rr, pcie2wb_hword;
    wire            pcie_in_status_read, pcie_in_data_read;
    wire [19:0]     pcie_in_addr;
    wire [31:0]     pcie2wb_payload;
    
    ioport2_msg_decode pcie_in_decoder (
        .message(msgi_tdata),
        .rd_response(pcie2wb_rr), .wr_request(pcie_in_wr), .rd_request(pcie_in_rd),
        .half_word(pcie2wb_hword), .address(pcie_in_addr), .data(pcie2wb_payload)
    );

    ioport2_msg_encode wb_out_decoder (
        .rd_response(pcie2wb_rr), .wr_request(wb_out_wr), .rd_request(wb_out_rd), 
        .half_word(pcie2wb_hword), .address(pcie_in_addr & PCIE_REGPORT_ADDR_MASK), .data(pcie2wb_payload),
        .message(wb_msgo_tdata)
    );
    
    assign wb_out_wr      = pcie_in_wr && ((pcie_in_addr & ~PCIE_REGPORT_ADDR_MASK) == PCIE_REGPORT_DATA_ADDR);
    assign wb_out_rd      = pcie_in_wr && ((pcie_in_addr & ~PCIE_REGPORT_ADDR_MASK) == PCIE_REGPORT_READ_ADDR);
    assign wb_msgo_tvalid = msgi_tvalid & (wb_out_wr | wb_out_rd | pcie2wb_rr);
    assign msgi_tready    = pcie_out_auto_resp_valid ? pcie_out_auto_resp_ready : wb_msgo_tready;

    //------------------------------------------
    
    //------------------------------------------
    // WB In -> PCIe Out
    //
    assign pcie_in_status_read = pcie_in_rd && ((pcie_in_addr & ~PCIE_REGPORT_ADDR_MASK) == PCIE_REGPORT_STATUS_ADDR);
    assign pcie_in_data_read   = pcie_in_rd && ((pcie_in_addr & ~PCIE_REGPORT_ADDR_MASK) == PCIE_REGPORT_DATA_ADDR);

    reg  [31:0]     wb_in_resp_payload_reg;

    wire            wb_in_rr, wb_msgi_tready_int;
    wire [63:0]     pcie_out_auto_resp_data;
    wire            pcie_out_auto_resp_valid, pcie_out_auto_resp_ready;
    wire [31:0]     wb_in_resp_payload;

    ioport2_msg_decode wb_in_decoder (
        .message(wb_msgi_tdata), .rd_response(wb_in_rr), .data(wb_in_resp_payload)
    );

    assign pcie_out_auto_resp_valid = (msgi_tvalid & (pcie_in_status_read | pcie_in_data_read));
    
    ioport2_msg_encode auto_response_encoder (
        .rd_response(1'b1), 
        .data(pcie_in_data_read ? wb_in_resp_payload_reg : {~wb_monitor_active, 30'h0, (wb_req_pending | wb_resp_pending)}),
        .message(pcie_out_auto_resp_data)
    );

    always @(posedge clk) begin
        if (rst)
            wb_in_resp_payload_reg <= 32'h0;
        else if (wb_msgi_tvalid & wb_msgi_tready & wb_in_rr)
            wb_in_resp_payload_reg <= wb_in_resp_payload;
    end

    axi_mux4 #(.PRIO(0), .WIDTH(64), .BUFFER(1)) msgo_arbiter_mux (
        .clk(clk), .reset(rst), .clear(1'b0),
        .i0_tdata(wb_msgi_tdata), .i0_tlast(1'b1), .i0_tvalid(wb_msgi_tvalid & ~wb_in_rr), .i0_tready(wb_msgi_tready_int),
        .i1_tdata(pcie_out_auto_resp_data), .i1_tlast(1'b1), .i1_tvalid(pcie_out_auto_resp_valid), .i1_tready(pcie_out_auto_resp_ready),
        .i2_tdata(0), .i2_tlast(1'b0), .i2_tvalid(1'b0), .i2_tready(),
        .i3_tdata(0), .i3_tlast(1'b0), .i3_tvalid(1'b0), .i3_tready(),
        .o_tdata(msgo_tdata), .o_tlast(), .o_tvalid(msgo_tvalid), .o_tready(msgo_tready)
    );

    assign wb_msgi_tready = wb_msgi_tready_int | (wb_msgi_tvalid & wb_in_rr);
    
    //------------------------------------------

endmodule
