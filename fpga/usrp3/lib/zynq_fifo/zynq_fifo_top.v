`timescale 1ns / 1ps

//////////////////////////////////////////////////////////////////////////////////
// Copyright Ettus Research LLC
// Copyright 2013 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// The ZYNQ FIFO Top:
//  - takes read and write 32-bit addressable AXI slave
//  - takes read and write 64-bit addressable AXI master
//  - drives N input and output AXI stream FIFOs
//////////////////////////////////////////////////////////////////////////////////

// Mapping:
// Page0 - stream to host config
// Page1 - host to stream config

module zynq_fifo_top
#(
    parameter CONFIG_BASE = 32'h40000000,
    parameter PAGE_WIDTH = 10, //in bytes, must fit widths below
    parameter H2S_STREAMS_WIDTH = 2,
    parameter H2S_CMDFIFO_DEPTH = 4,
    parameter S2H_STREAMS_WIDTH = 2,
    parameter S2H_CMDFIFO_DEPTH = 4,
    parameter PROT = 3'b010 //data, non-secure, unpriv
)
(
    input clk,
    input rst,

    //------------------------------------------------------------------
    // AXI slave addressable signals - control
    //------------------------------------------------------------------
    //control write signals - slave
    input [31:0] CTL_AXI_AWADDR,
    input CTL_AXI_AWVALID,
    output CTL_AXI_AWREADY,
    input [31:0] CTL_AXI_WDATA,
    input [3:0] CTL_AXI_WSTRB,
    input CTL_AXI_WVALID,
    output CTL_AXI_WREADY,
    output [1:0] CTL_AXI_BRESP,
    output CTL_AXI_BVALID,
    input CTL_AXI_BREADY,

    //control read signals - slave
    input [31:0] CTL_AXI_ARADDR,
    input CTL_AXI_ARVALID,
    output CTL_AXI_ARREADY,
    output [31:0] CTL_AXI_RDATA,
    output [1:0] CTL_AXI_RRESP,
    output CTL_AXI_RVALID,
    input CTL_AXI_RREADY,

    //------------------------------------------------------------------
    // AXI master addressable signals - DDR access
    //------------------------------------------------------------------
    //memory write signals - master
    output [5:0] DDR_AXI_AWID,
    output [31:0] DDR_AXI_AWADDR,
    output [2:0] DDR_AXI_AWPROT,
    output DDR_AXI_AWVALID,
    input DDR_AXI_AWREADY,
    output [63:0] DDR_AXI_WDATA,
    output [7:0] DDR_AXI_WSTRB,
    output DDR_AXI_WVALID,
    input DDR_AXI_WREADY,
    input [1:0] DDR_AXI_BRESP,
    input DDR_AXI_BVALID,
    output DDR_AXI_BREADY,
    output [7:0] DDR_AXI_AWLEN,
    output [2:0] DDR_AXI_AWSIZE,
    output [1:0] DDR_AXI_AWBURST,
    output [3:0] DDR_AXI_AWCACHE,
    output DDR_AXI_WLAST,

    //memory read signals - master
    output [5:0] DDR_AXI_ARID,
    output [31:0] DDR_AXI_ARADDR,
    output [2:0] DDR_AXI_ARPROT,
    output DDR_AXI_ARVALID,
    input DDR_AXI_ARREADY,
    input [63:0] DDR_AXI_RDATA,
    input [1:0] DDR_AXI_RRESP,
    input DDR_AXI_RVALID,
    output DDR_AXI_RREADY,
    input DDR_AXI_RLAST,
    output [3:0] DDR_AXI_ARCACHE,
    output [7:0] DDR_AXI_ARLEN,
    output [1:0] DDR_AXI_ARBURST,
    output [2:0] DDR_AXI_ARSIZE,

    //------------------------------------------------------------------
    // AXI streams host to stream
    //------------------------------------------------------------------
    output [63:0] h2s_tdata,
    output h2s_tlast,
    output h2s_tvalid,
    input h2s_tready,

    //------------------------------------------------------------------
    // AXI streams stream to host
    //------------------------------------------------------------------
    input [63:0] s2h_tdata,
    input s2h_tlast,
    input s2h_tvalid,
    output s2h_tready,

    output event_irq,

    //------------------------------------------------------------------
    // Settings bus interface that will got to e300 core
    //------------------------------------------------------------------
    output [31:0] core_set_data,
    output [7:0]  core_set_addr,
    output        core_set_stb,
    input [31:0]  core_rb_data,

    //------------------------------------------------------------------
    // Settings bus interface for crossbar (in e300 core)
    //------------------------------------------------------------------
    output [31:0] xbar_set_data,
    output [31:0] xbar_set_addr,
    output        xbar_set_stb,
    input [31:0]  xbar_rb_data,
    output [31:0] xbar_rb_addr,
    output        xbar_rb_stb,

    output [31:0] debug
);

////////////////////////////////////////////////////////////////////////
///////////////////////////// Begin R T L //////////////////////////////
////////////////////////////////////////////////////////////////////////

    //interrupt wires
    wire h2s_irq, s2h_irq;
    assign event_irq = h2s_irq | s2h_irq;

    wire [31:0] set_addr, set_data;
    wire [31:0] rb_addr, rb_data;
    wire [31:0] rb_data_s2h, rb_data_h2s;
    wire set_stb, set_stb_s2h, set_stb_h2s, set_stb_dest_lookup;
    wire rb_stb, rb_stb_s2h, rb_stb_h2s;

    wire [2:0] set_page = set_addr[PAGE_WIDTH+2:PAGE_WIDTH];
    wire [2:0] rb_page = rb_addr[PAGE_WIDTH+2:PAGE_WIDTH];

    // each arbiter gets 1 page, e300_core the next,
    // destination lookup, and xbar gets two pages
    assign set_stb_s2h         = set_stb && (set_page == 3'h0);
    assign set_stb_h2s         = set_stb && (set_page == 3'h1);
    assign core_set_stb        = set_stb && (set_page == 3'h2);
    assign set_stb_dest_lookup = set_stb && (set_page == 3'h3);
    assign xbar_set_stb        = set_stb && (set_page == 3'h4 || set_page == 3'h5);

    assign rb_stb_s2h = rb_stb && (rb_page == 3'h0);
    assign rb_stb_h2s = rb_stb && (rb_page == 3'h1);
    assign xbar_rb_stb = rb_stb && (rb_page == 3'h4);
    assign rb_data = (rb_page == 3'h0)? rb_data_s2h :
                     (rb_page == 3'h1)? rb_data_h2s :
                     (rb_page == 3'h2)? core_rb_data:
                     // no readback for dest_lookup
                     (rb_page == 3'h4)? xbar_rb_data:
                     32'hdeadbeef;

    assign core_set_addr = set_addr[7:0];
    assign core_set_data = set_data;

    assign xbar_rb_addr = rb_addr[10:0];
    assign xbar_set_data = set_data;
    assign xbar_set_addr = set_addr[10:0];

    //------------------------------------------------------------------
    // configuration slaves
    //------------------------------------------------------------------
    zf_slave_settings #(.CONFIG_BASE(CONFIG_BASE)) zf_slave_settings
    (
        .clk(clk), .rst(rst),
        .AXI_AWADDR(CTL_AXI_AWADDR),
        .AXI_AWVALID(CTL_AXI_AWVALID),
        .AXI_AWREADY(CTL_AXI_AWREADY),
        .AXI_WDATA(CTL_AXI_WDATA),
        .AXI_WSTRB(CTL_AXI_WSTRB),
        .AXI_WVALID(CTL_AXI_WVALID),
        .AXI_WREADY(CTL_AXI_WREADY),
        .AXI_BRESP(CTL_AXI_BRESP),
        .AXI_BVALID(CTL_AXI_BVALID),
        .AXI_BREADY(CTL_AXI_BREADY),
        .addr(set_addr), .data(set_data), .strobe(set_stb),
        .debug()
    );
    zf_slave_readback #(.CONFIG_BASE(CONFIG_BASE)) zf_slave_readback
    (
        .clk(clk), .rst(rst),
        .AXI_ARADDR(CTL_AXI_ARADDR),
        .AXI_ARVALID(CTL_AXI_ARVALID),
        .AXI_ARREADY(CTL_AXI_ARREADY),
        .AXI_RDATA(CTL_AXI_RDATA),
        .AXI_RRESP(CTL_AXI_RRESP),
        .AXI_RVALID(CTL_AXI_RVALID),
        .AXI_RREADY(CTL_AXI_RREADY),
        .addr(rb_addr), .data(rb_data), .strobe(rb_stb),
        .debug()
    );

    //------------------------------------------------------------------
    // fifo to ddr
    //------------------------------------------------------------------
    wire [71:0] s2h_cmd_tdata;
    wire [7:0] s2h_sts_tdata;
    wire s2h_cmd_tvalid, s2h_cmd_tready;
    wire s2h_sts_tvalid, s2h_sts_tready;

    assign s2h_irq = s2h_sts_tvalid;

    //lookup destination
    wire [63:0] s2h_tdata_i0;
    wire s2h_tready_i0, s2h_tvalid_i0, s2h_tlast_i0;
    wire [S2H_STREAMS_WIDTH-1:0] which_stream_s2h;

    cvita_dest_lookup #(.DEST_WIDTH(S2H_STREAMS_WIDTH)) s2h_dest_gen
    (
        .clk(clk), .rst(rst),
        .set_stb(set_stb_dest_lookup), .set_addr(set_addr[9:2]), .set_data(set_data),
        .i_tdata(s2h_tdata), .i_tlast(s2h_tlast), .i_tvalid(s2h_tvalid), .i_tready(s2h_tready),
        .o_tdata(s2h_tdata_i0), .o_tlast(s2h_tlast_i0), .o_tvalid(s2h_tvalid_i0), .o_tready(s2h_tready_i0),
        .o_tdest(which_stream_s2h)
    );

    //only active in cycles between command and tlast
    //this prevents bullshit consumption after tlast
    reg s2h_active;
    always @(posedge clk) begin
        if (rst) s2h_active <= 0;
        else if (s2h_cmd_tvalid && s2h_cmd_tready) s2h_active <= 1;
        else if (s2h_tready_i0 && s2h_tvalid_i0 && s2h_tlast_i0) s2h_active <= 0;
    end

    //cut fifo comms when not in active state
    wire [63:0] s2h_tdata_i1;
    wire s2h_tready_i1, s2h_tvalid_i1, s2h_tlast_i1;
    assign s2h_tdata_i1 = s2h_tdata_i0;
    assign s2h_tlast_i1 = s2h_tlast_i0;
    assign s2h_tvalid_i1 = s2h_tvalid_i0 && s2h_active;
    assign s2h_tready_i0 = s2h_tready_i1 && s2h_active;

    wire [31:0] s2h_arbiter_debug;
    zf_arbiter #(
        .STREAMS_WIDTH(H2S_STREAMS_WIDTH),
        .CMDFIFO_DEPTH(H2S_CMDFIFO_DEPTH),
        .PAGE_WIDTH(PAGE_WIDTH)
    ) s2h_arbiter
    (
        .clk(clk), .rst(rst),
        .set_addr(set_addr), .set_data(set_data), .set_stb(set_stb_s2h),
        .rb_addr(rb_addr), .rb_data(rb_data_s2h), .rb_stb(rb_stb_s2h),
        .cmd_tdata(s2h_cmd_tdata), .cmd_tvalid(s2h_cmd_tvalid), .cmd_tready(s2h_cmd_tready),
        .sts_tdata(s2h_sts_tdata), .sts_tvalid(s2h_sts_tvalid), .sts_tready(s2h_sts_tready),
        .ext_stream_sel(which_stream_s2h), .ext_stream_valid(s2h_tvalid_i0),
        .debug(s2h_arbiter_debug)
    );

    //------------------------------------------------------------------
    // ddr to fifo
    //------------------------------------------------------------------
    wire [71:0] h2s_cmd_tdata;
    wire [7:0] h2s_sts_tdata;
    wire h2s_cmd_tvalid, h2s_cmd_tready;
    wire h2s_sts_tvalid, h2s_sts_tready;

    assign h2s_irq = h2s_sts_tvalid;

    wire [31:0] h2s_arbiter_debug;
    zf_arbiter #(
        .STREAMS_WIDTH(S2H_STREAMS_WIDTH),
        .CMDFIFO_DEPTH(S2H_CMDFIFO_DEPTH),
        .PAGE_WIDTH(PAGE_WIDTH),
        .USE_INT_STREAM_SEL(1)
    ) h2s_arbiter
    (
        .clk(clk), .rst(rst),
        .set_addr(set_addr), .set_data(set_data), .set_stb(set_stb_h2s),
        .rb_addr(rb_addr), .rb_data(rb_data_h2s), .rb_stb(rb_stb_h2s),
        .cmd_tdata(h2s_cmd_tdata), .cmd_tvalid(h2s_cmd_tvalid), .cmd_tready(h2s_cmd_tready),
        .sts_tdata(h2s_sts_tdata), .sts_tvalid(h2s_sts_tvalid), .sts_tready(h2s_sts_tready),
        .ext_stream_sel(), .ext_stream_valid(),
        .debug(h2s_arbiter_debug)
    );

    //------------------------------------------------------------------
    // axi_datamover
    //------------------------------------------------------------------
    wire reset_dm = rst;
    axi_dma_stream inst_axi_dma_stream
    (
        //host to stream reset stuff
        .m_axi_mm2s_aclk(clk),
        .m_axi_mm2s_aresetn(!reset_dm),
        .mm2s_halt(1'b0),
        .mm2s_halt_cmplt(),
        .mm2s_err(),

        //host to stream command
        .m_axis_mm2s_cmdsts_aclk(clk),
        .m_axis_mm2s_cmdsts_aresetn(!reset_dm),
        .s_axis_mm2s_cmd_tvalid(h2s_cmd_tvalid),
        .s_axis_mm2s_cmd_tready(h2s_cmd_tready),
        .s_axis_mm2s_cmd_tdata(h2s_cmd_tdata),

        //host to stream status
        .m_axis_mm2s_sts_tvalid(h2s_sts_tvalid),
        .m_axis_mm2s_sts_tready(h2s_sts_tready),
        .m_axis_mm2s_sts_tdata(h2s_sts_tdata),
        .m_axis_mm2s_sts_tkeep(),
        .m_axis_mm2s_sts_tlast(),

        //store and forward - always can post
        .mm2s_allow_addr_req(1'b1),
        .mm2s_addr_req_posted(),
        .mm2s_rd_xfer_cmplt(),

        //HP RD connection to DDR
        .m_axi_mm2s_arid(DDR_AXI_ARID),
        .m_axi_mm2s_araddr(DDR_AXI_ARADDR),
        .m_axi_mm2s_arlen(DDR_AXI_ARLEN),
        .m_axi_mm2s_arsize(DDR_AXI_ARSIZE),
        .m_axi_mm2s_arburst(DDR_AXI_ARBURST),
        .m_axi_mm2s_arprot(DDR_AXI_ARPROT),
        .m_axi_mm2s_arcache(DDR_AXI_ARCACHE),
        .m_axi_mm2s_aruser(),
        .m_axi_mm2s_arvalid(DDR_AXI_ARVALID),
        .m_axi_mm2s_arready(DDR_AXI_ARREADY),
        .m_axi_mm2s_rdata(DDR_AXI_RDATA),
        .m_axi_mm2s_rresp(DDR_AXI_RRESP),
        .m_axi_mm2s_rlast(DDR_AXI_RLAST),
        .m_axi_mm2s_rvalid(DDR_AXI_RVALID),
        .m_axi_mm2s_rready(DDR_AXI_RREADY),

        //AXI host to stream connection
        .m_axis_mm2s_tdata({h2s_tdata[31:0], h2s_tdata[63:32]}),
        .m_axis_mm2s_tkeep(), //dont care
        .m_axis_mm2s_tlast(h2s_tlast),
        .m_axis_mm2s_tvalid(h2s_tvalid),
        .m_axis_mm2s_tready(h2s_tready),

        //unused debug
        .mm2s_dbg_sel(4'b0),
        .mm2s_dbg_data(),

        //stream to host reset stuff
        .m_axi_s2mm_aclk(clk),
        .m_axi_s2mm_aresetn(!reset_dm),
        .s2mm_halt(1'b0),
        .s2mm_halt_cmplt(),
        .s2mm_err(),

        //stream to host command
        .m_axis_s2mm_cmdsts_awclk(clk),
        .m_axis_s2mm_cmdsts_aresetn(!reset_dm),
        .s_axis_s2mm_cmd_tvalid(s2h_cmd_tvalid),
        .s_axis_s2mm_cmd_tready(s2h_cmd_tready),
        .s_axis_s2mm_cmd_tdata(s2h_cmd_tdata),

        //stream to host status
        .m_axis_s2mm_sts_tvalid(s2h_sts_tvalid),
        .m_axis_s2mm_sts_tready(s2h_sts_tready),
        .m_axis_s2mm_sts_tdata(s2h_sts_tdata),
        .m_axis_s2mm_sts_tkeep(),
        .m_axis_s2mm_sts_tlast(),

        //store and forward - always can post
        .s2mm_allow_addr_req(1'b1),
        .s2mm_addr_req_posted(),
        .s2mm_wr_xfer_cmplt(),
        .s2mm_ld_nxt_len(),
        .s2mm_wr_len(),

        //HP WR connection to DDR
        .m_axi_s2mm_awid(DDR_AXI_AWID),
        .m_axi_s2mm_awaddr(DDR_AXI_AWADDR),
        .m_axi_s2mm_awlen(DDR_AXI_AWLEN),
        .m_axi_s2mm_awsize(DDR_AXI_AWSIZE),
        .m_axi_s2mm_awburst(DDR_AXI_AWBURST),
        .m_axi_s2mm_awprot(DDR_AXI_AWPROT),
        .m_axi_s2mm_awcache(DDR_AXI_AWCACHE),
        .m_axi_s2mm_awuser(),
        .m_axi_s2mm_awvalid(DDR_AXI_AWVALID),
        .m_axi_s2mm_awready(DDR_AXI_AWREADY),
        .m_axi_s2mm_wdata(DDR_AXI_WDATA),
        .m_axi_s2mm_wstrb(DDR_AXI_WSTRB),
        .m_axi_s2mm_wlast(DDR_AXI_WLAST),
        .m_axi_s2mm_wvalid(DDR_AXI_WVALID),
        .m_axi_s2mm_wready(DDR_AXI_WREADY),
        .m_axi_s2mm_bresp(DDR_AXI_BRESP),
        .m_axi_s2mm_bvalid(DDR_AXI_BVALID),
        .m_axi_s2mm_bready(DDR_AXI_BREADY),

        //AXI stream to host connection
        .s_axis_s2mm_tdata({s2h_tdata_i1[31:0], s2h_tdata_i1[63:32]}),
        .s_axis_s2mm_tkeep(8'hff), //all bytes valid
        .s_axis_s2mm_tlast(s2h_tlast_i1),
        .s_axis_s2mm_tvalid(s2h_tvalid_i1),
        .s_axis_s2mm_tready(s2h_tready_i1),

        //unused debug
        .s2mm_dbg_sel(4'b0),
        .s2mm_dbg_data()
    );

    //------------------------------------------------------------------
    // chipscope debugs
    //------------------------------------------------------------------
/*  wire [35:0] CONTROL;
    wire [255:0] DATA;
    wire [7:0] TRIG;

    chipscope_icon chipscope_icon(.CONTROL0(CONTROL));

    chipscope_ila chipscope_ila
    (
        .CONTROL(CONTROL), .CLK(clk),
        .DATA(DATA), .TRIG0(TRIG)
    );

    assign DATA[255:256-64] = {set_addr, set_data};
    assign DATA[79:76] = which_stream_s2h;
    assign DATA[75:72] = {1'b0, s2h_tlast, s2h_tvalid, s2h_tready};
    assign DATA[71:68] = {1'b0, s2h_tlast_i0, s2h_tvalid_i0, s2h_tready_i0};
    assign DATA[67:64] = {1'b0, s2h_tlast_i1, s2h_tvalid_i1, s2h_tready_i1};
    assign DATA[63:0] = s2h_tdata_i1;

    assign TRIG = {
        set_stb, s2h_tlast_i0, s2h_tvalid_i0, s2h_tready_i0,
        (set_stb && (set_page == 2'h2)), s2h_tlast_i1, s2h_tvalid_i1, s2h_tready_i1
    };
*/
endmodule //zynq_fifo_top
