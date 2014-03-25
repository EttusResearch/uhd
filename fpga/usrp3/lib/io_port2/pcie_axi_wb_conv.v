//
// Copyright 2013 Ettus Research LLC
//


module pcie_axi_wb_conv #(
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
    
    output                  wb_monitor_active,
    output reg              wb_req_pending,
    output reg              wb_resp_pending,
    output reg              pcie_resp_pending
);

    localparam SB_ADDRW                 = 4;
    localparam SB_DATAW                 = 32;
    localparam MONITOR_TIMEOUTW         = 20;   //20bits@175MHz ~ 6ms

    localparam SR_PCIE_DATA_REG         = 4'd0;
    localparam SR_PCIE_CTRL_REG         = 4'd1;
    localparam RB_PCIE_DATA_REG         = 4'd0;
    localparam RB_PCIE_CTRL_REG         = 4'd1;
    localparam RB_PCIE_RESP_DATA_REG    = 4'd2;
    localparam RB_PCIE_STATUS_REG       = 4'd3;

    //------------------------------------------
    // Settings and readback bus
    //
    wire [SB_DATAW-1:0]     set_data, rb_data;
    wire [SB_ADDRW-1:0]     set_addr, rb_addr;
    wire                    set_stb, rb_stb;

    settings_bus #(.AWIDTH(WB_ADDRW), .DWIDTH(WB_DATAW)) settings_bus (
        .wb_clk(clk), .wb_rst(rst),
        .wb_adr_i(wb_adr_i), .wb_dat_i(wb_dat_i),
        .wb_stb_i(wb_stb_i), .wb_we_i(wb_we_i), .wb_ack_o(wb_ack_o),
        .strobe(set_stb), .addr(set_addr), .data(set_data)
    );

    settings_readback #(.AWIDTH(WB_ADDRW), .DWIDTH(WB_DATAW), .RB_ADDRW(SB_ADDRW)) settings_readback (
        .wb_clk(clk), .wb_rst(rst), 
        .wb_adr_i(wb_adr_i), .wb_stb_i(wb_stb_i), .wb_we_i(wb_we_i),
        .rb_data(rb_data), .rb_addr(rb_addr), .rb_rd_stb(rb_stb),
        .wb_dat_o(wb_dat_o)
    );
    //------------------------------------------
    
    //------------------------------------------
    // Settings/Readback Registers
    //
    wire [31:0]     axi_out_data, axi_out_ctrl;
    wire            axi_out_stb;

    setting_reg #(.my_addr(SR_PCIE_DATA_REG), .awidth(SB_ADDRW), .width(SB_DATAW)) set_pcie_out_data_reg (
        .clk(clk), .rst(rst),
        .strobe(set_stb), .addr(set_addr), .in(set_data),
        .out(axi_out_data)
    );

    setting_reg #(.my_addr(SR_PCIE_CTRL_REG), .awidth(SB_ADDRW), .width(SB_DATAW)) set_pcie_out_ctrl_reg (
        .clk(clk), .rst(rst),
        .strobe(set_stb), .addr(set_addr), .in(set_data),
        .out(axi_out_ctrl), .changed(axi_out_stb)
    );

    reg [31:0]      axi_in_data_reg, axi_in_ctrl_reg, axi_in_resp_reg;
    wire            msgo_fifo_tready;

    // Readback MUX
    assign rb_data = (
        (rb_addr == RB_PCIE_STATUS_REG)    ? {27'h0, ~msgo_fifo_tready, 1'b0, wb_resp_pending, wb_req_pending, pcie_resp_pending} : (
        (rb_addr == RB_PCIE_RESP_DATA_REG) ? axi_in_resp_reg : (
        (rb_addr == RB_PCIE_DATA_REG)      ? axi_in_data_reg : (
        (rb_addr == RB_PCIE_CTRL_REG)      ? axi_in_ctrl_reg : 32'h0))));
    //------------------------------------------

    //------------------------------------------
    // Output message handler
    //
    wire [63:0]     msgo_fifo_tdata;
    wire            axi_out_rd, axi_out_wr, axi_out_rr;

    assign msgo_fifo_tdata = {axi_out_ctrl, axi_out_data};

    axi_fifo_short #(.WIDTH(64)) wb_out_msg_fifo (
        .clk(clk), .reset(rst), .clear(1'b0),
        .i_tdata(msgo_fifo_tdata), .i_tvalid(axi_out_stb), .i_tready(msgo_fifo_tready),
        .o_tdata(msgo_tdata), .o_tvalid(msgo_tvalid), .o_tready(msgo_tready),
        .space(), .occupied());

    ioport2_msg_decode axi_out_decoder (
        .message(msgo_fifo_tdata),
        .rd_response(axi_out_rr), .wr_request(axi_out_wr), .rd_request(axi_out_rd)
    );
    //------------------------------------------
    
    //------------------------------------------
    // Input message handler
    //
    wire [63:0]     msgi_fifo_tdata;
    wire            axi_in_valid, axi_in_stb;

    wire [31:0]     axi_in_data, axi_in_ctrl;
    wire            axi_in_rd, axi_in_wr, axi_in_rr;
    
    axi_fifo_short #(.WIDTH(64)) wb_in_msg_fifo (
        .clk(clk), .reset(rst), .clear(1'b0),
        .i_tdata(msgi_tdata), .i_tvalid(msgi_tvalid), .i_tready(msgi_tready),
        .o_tdata(msgi_fifo_tdata), .o_tvalid(axi_in_valid), .o_tready(axi_in_stb),
        .space(), .occupied());

    ioport2_msg_decode axi_in_decoder (
        .message(msgi_fifo_tdata),
        .rd_response(axi_in_rr), .wr_request(axi_in_wr), .rd_request(axi_in_rd),
        .data(axi_in_data), .control(axi_in_ctrl)
    );

    assign axi_in_stb = axi_in_valid & (axi_in_rr | ((axi_in_wr | axi_in_rd) &  ~(wb_req_pending | wb_resp_pending)));

    always @(posedge clk) begin
        if (rst) begin
            axi_in_data_reg     <= 32'h0;
            axi_in_ctrl_reg     <= 32'h0;
            axi_in_resp_reg     <= 32'h0;
        end else begin
            if (axi_in_stb & axi_in_rr) begin
                axi_in_resp_reg <= axi_in_data;
            end else if (axi_in_stb & (axi_in_wr | axi_in_rd)) begin
                axi_in_data_reg <= axi_in_data;
                axi_in_ctrl_reg <= axi_in_ctrl;
            end
        end
    end
    //------------------------------------------
    
    //------------------------------------------
    // State handler
    //
    //wb_monitor_active
    reg [MONITOR_TIMEOUTW-1:0]  wb_monitor_timeout;
    assign wb_monitor_active = (wb_monitor_timeout != {(MONITOR_TIMEOUTW){1'b0}});
    always @(posedge clk) begin
        if (rst) 
            wb_monitor_timeout  <= {(MONITOR_TIMEOUTW){1'b0}}; //Monitor disabled on rst
        else if (rb_stb && (rb_addr == RB_PCIE_STATUS_REG)) 
            wb_monitor_timeout  <= {(MONITOR_TIMEOUTW){1'b1}}; //Reset counter when the ZPU queries the status reg
        else if (wb_monitor_active)
            wb_monitor_timeout  <= wb_monitor_timeout - 1;  //Decrement counter when idle
    end

    //wb_req_pending
    always @(posedge clk) begin
        if (rst || (rb_stb && (rb_addr == RB_PCIE_CTRL_REG)))
            wb_req_pending  <= 1'b0;
        else if (axi_in_stb & (axi_in_rd | axi_in_wr)) 
            wb_req_pending  <= 1'b1;
    end
    
    //wb_resp_pending
    always @(posedge clk) begin
        if (rst | (axi_out_stb & msgo_fifo_tready & axi_out_rr))
            wb_resp_pending  <= 1'b0;
        else if (axi_in_stb & axi_in_rd) 
            wb_resp_pending  <= 1'b1;
    end

    //pcie_resp_pending
    always @(posedge clk) begin
        if (rst | (axi_in_stb & axi_in_rr))
            pcie_resp_pending  <= 1'b0;
        else if (axi_out_stb & msgo_fifo_tready & axi_out_rd) 
            pcie_resp_pending  <= 1'b1;
    end
    //------------------------------------------


endmodule
