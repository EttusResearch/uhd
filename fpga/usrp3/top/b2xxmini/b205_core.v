//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


/***********************************************************
 * B205 Core Guts
 **********************************************************/
module b205_core
#(
    parameter R0_CTRL_SID = 8'h10,
    parameter U0_CTRL_SID = 8'h30,
    parameter L0_CTRL_SID = 8'h40,
    parameter R0_DATA_SID = 8'h50,
    parameter R1_DATA_SID = 8'h60,
    parameter DEMUX_SID_MASK = 8'hf0,
    parameter EXTRA_BUFF_SIZE = 0,
    parameter RADIO_FIFO_SIZE = 11,
    parameter SAMPLE_FIFO_SIZE = 11

)
(
    ////////////////////////////////////////////////////////////////////
    // bus interfaces
    ////////////////////////////////////////////////////////////////////
    input 	  bus_clk,
    input 	  bus_rst,

    input [63:0]  tx_tdata, input tx_tlast, input tx_tvalid, output tx_tready,
    output [63:0] rx_tdata, output rx_tlast, output rx_tvalid, input rx_tready,
    input [63:0]  ctrl_tdata, input ctrl_tlast, input ctrl_tvalid, output ctrl_tready,
    output [63:0] resp_tdata, output resp_tlast, output resp_tvalid, input resp_tready,

    ////////////////////////////////////////////////////////////////////
    // radio interfaces
    ////////////////////////////////////////////////////////////////////
    input 	  radio_clk,
    input 	  radio_rst,

    input [31:0]  rx0,
    output [31:0] tx0,
    output [7:0] fe_gpio_out,
    input [7:0] fp_gpio_in, output [7:0] fp_gpio_out, output [7:0] fp_gpio_ddr,
    output ext_ref_is_pps,
    input pps_ext,

    ////////////////////////////////////////////////////////////////////
    // core interfaces
    ////////////////////////////////////////////////////////////////////
    output [7:0]  sen, output sclk, output mosi, input miso,
    input [31:0]  rb_misc,
    output [31:0] misc_outs,

    ////////////////////////////////////////////////////////////////////
    // fe lock signals
    ////////////////////////////////////////////////////////////////////
    input [1:0] lock_signals,

    ////////////////////////////////////////////////////////////////////
    // debug signals
    ////////////////////////////////////////////////////////////////////
    output [63:0] debug
);
    localparam SR_CORE_SPI       = 8'd8;
    localparam SR_CORE_MISC      = 8'd16;
    localparam SR_CORE_COMPAT    = 8'd24;
    localparam SR_CORE_READBACK  = 8'd32;
    localparam SR_CORE_GPSDO_ST  = 8'd40;
    localparam SR_CORE_SYNC      = 8'd48;
    localparam COMPAT_MAJOR      = 16'h0007;
    localparam COMPAT_MINOR      = 16'h0000;

    reg [1:0] lock_state;
    reg [1:0] lock_state_r;

    always @(posedge bus_clk)
      if (bus_rst)
        {lock_state_r, lock_state} <= 4'h0;
      else
        {lock_state_r, lock_state} <= {lock_state, lock_signals};


    /*******************************************************************
     * PPS Timing stuff
     ******************************************************************/

    // Generate an internal PPS signal
    wire int_pps;
    pps_generator #(.CLK_FREQ(100000000)) pps_gen
    (.clk(bus_clk), .pps(int_pps));

    // Flop PPS signals into radio clock domain
    reg [1:0] 	 ext_pps_del, int_pps_del;
    always @(posedge radio_clk) ext_pps_del[1:0] <= {ext_pps_del[0], pps_ext};
    always @(posedge radio_clk) int_pps_del[1:0] <= {int_pps_del[0], int_pps};

    // PPS mux
    wire [1:0] pps_select;
    wire pps =  (pps_select == 2'b01)? ext_pps_del[1] :
                (pps_select == 2'b10)? int_pps_del[1] :
                1'b0;
    assign ext_ref_is_pps = (pps_select == 2'b01);

    /*******************************************************************
     * Response mux Routing logic
     ******************************************************************/
    wire [63:0] r0_resp_tdata; wire r0_resp_tlast, r0_resp_tvalid, r0_resp_tready;
    wire [63:0] l0_resp_tdata; wire l0_resp_tlast, l0_resp_tvalid, l0_resp_tready;

    axi_mux4 #(.WIDTH(64), .BUFFER(1)) mux_for_resp
    (
        .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
        .i0_tdata(r0_resp_tdata), .i0_tlast(r0_resp_tlast), .i0_tvalid(r0_resp_tvalid), .i0_tready(r0_resp_tready),
        .i1_tdata(l0_resp_tdata), .i1_tlast(l0_resp_tlast), .i1_tvalid(l0_resp_tvalid), .i1_tready(l0_resp_tready),
        .i2_tdata(64'd0), .i2_tlast(1'b0), .i2_tvalid(1'b0), .i2_tready(),
        .i3_tdata(64'd0), .i3_tlast(1'b0), .i3_tvalid(1'b0), .i3_tready(),
        .o_tdata(resp_tdata), .o_tlast(resp_tlast), .o_tvalid(resp_tvalid), .o_tready(resp_tready)
    );

    /*******************************************************************
     * Control demux Routing logic
     ******************************************************************/
    wire [63:0] r0_ctrl_tdata; wire r0_ctrl_tlast, r0_ctrl_tvalid, r0_ctrl_tready;
    wire [63:0] l0_ctrl_tdata; wire l0_ctrl_tlast, l0_ctrl_tvalid, l0_ctrl_tready;

    wire [63:0] ctrl_hdr;
    wire [1:0] ctrl_dst =
        ((ctrl_hdr[7:0] & DEMUX_SID_MASK) == R0_CTRL_SID)? 0 : (
        ((ctrl_hdr[7:0] & DEMUX_SID_MASK) == L0_CTRL_SID)? 1 : (
    3));
    axi_demux4 #(.ACTIVE_CHAN(4'b1111), .WIDTH(64), .BUFFER(1)) demux_for_ctrl
    (
        .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
        .header(ctrl_hdr), .dest(ctrl_dst),
        .i_tdata(ctrl_tdata), .i_tlast(ctrl_tlast), .i_tvalid(ctrl_tvalid), .i_tready(ctrl_tready),
        .o0_tdata(r0_ctrl_tdata), .o0_tlast(r0_ctrl_tlast), .o0_tvalid(r0_ctrl_tvalid), .o0_tready(r0_ctrl_tready),
        .o1_tdata(l0_ctrl_tdata), .o1_tlast(l0_ctrl_tlast), .o1_tvalid(l0_ctrl_tvalid), .o1_tready(l0_ctrl_tready),
        .o2_tdata(), .o2_tlast(), .o2_tvalid(), .o2_tready(1'b1),
        .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b1)
    );

    /*******************************************************************
     * Misc controls
     ******************************************************************/
    wire 	set_stb;
    wire [7:0] 	set_addr;
    wire [31:0] 	set_data;

    wire spi_ready;
    wire [31:0] spi_readback;

    wire [1:0] rb_addr;
    reg [63:0] rb_data;

    wire [63:0] l0i_ctrl_tdata; wire l0i_ctrl_tlast, l0i_ctrl_tvalid, l0i_ctrl_tready;

    wire time_sync, time_sync_r;

    synchronizer time_sync_synchronizer
     (.clk(radio_clk), .rst(radio_rst), .in(time_sync), .out(time_sync_r));

    axi_fifo #(.WIDTH(65), .SIZE(0)) radio_ctrl_proc_timing_fifo
    (
        .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
        .i_tdata({l0_ctrl_tlast, l0_ctrl_tdata}), .i_tvalid(l0_ctrl_tvalid), .i_tready(l0_ctrl_tready), .space(),
        .o_tdata({l0i_ctrl_tlast, l0i_ctrl_tdata}), .o_tvalid(l0i_ctrl_tvalid), .o_tready(l0i_ctrl_tready), .occupied()
    );

    radio_ctrl_proc radio_ctrl_proc
    (
        .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
        .ctrl_tdata(l0i_ctrl_tdata), .ctrl_tlast(l0i_ctrl_tlast), .ctrl_tvalid(l0i_ctrl_tvalid), .ctrl_tready(l0i_ctrl_tready),
        .resp_tdata(l0_resp_tdata), .resp_tlast(l0_resp_tlast), .resp_tvalid(l0_resp_tvalid), .resp_tready(l0_resp_tready),
        .vita_time(64'b0),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .ready(spi_ready), .readback(rb_data),
        .debug()
    );

    setting_reg #(.my_addr(SR_CORE_MISC), .awidth(8), .width(32), .at_reset(8'h0)) sr_misc
    (
        .clk(bus_clk), .rst(bus_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
        .out(misc_outs), .changed()
    );

    setting_reg #(.my_addr(SR_CORE_READBACK), .awidth(8), .width(2)) sr_rdback
    (
        .clk(bus_clk), .rst(bus_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
        .out(rb_addr), .changed()
    );

    setting_reg #(.my_addr(SR_CORE_SYNC), .awidth(8), .width(3)) sr_sync
    (
        .clk(bus_clk), .rst(bus_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
        .out({time_sync,pps_select}), .changed()
    );

    simple_spi_core #(.BASE(SR_CORE_SPI), .WIDTH(8), .CLK_IDLE(0), .SEN_IDLE(8'hFF)) misc_spi
    (
        .clock(bus_clk), .reset(bus_rst),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .readback(spi_readback), .ready(spi_ready),
        .sen(sen), .sclk(sclk), .mosi(mosi), .miso(miso),
        .debug()
    );

    always @*
     case(rb_addr)
       2'd0 : rb_data <= { 32'hACE0BA5E, COMPAT_MAJOR, COMPAT_MINOR };
       2'd1 : rb_data <= { 32'b0, spi_readback };
       2'd2 : rb_data <= { 16'b0, 8'd1, 8'd0, rb_misc };
       2'd3 : rb_data <= { 30'h0, lock_state_r };
       default : rb_data <= 64'd0;
     endcase // case (rb_addr)

    /*******************************************************************
     * Radio
     ******************************************************************/
    wire [31:0] fe_gpio_out32;
    wire [9:0] fp_gpio_out10, fp_gpio_ddr10;
    assign fe_gpio_out = fe_gpio_out32[7:0];
    assign fp_gpio_out = fp_gpio_out10[7:0];
    assign fp_gpio_ddr = fp_gpio_ddr10[7:0];

    radio_legacy #(
        .RADIO_FIFO_SIZE(RADIO_FIFO_SIZE),
        .SAMPLE_FIFO_SIZE(SAMPLE_FIFO_SIZE),
        .FP_GPIO(1),
        .NEW_HB_INTERP(1),
        .NEW_HB_DECIM(1),
        .SOURCE_FLOW_CONTROL(0),
        .USER_SETTINGS(0),
        .DEVICE("SPARTAN6")
    ) radio (
        .radio_clk(radio_clk), .radio_rst(radio_rst),
        .rx(rx0), .tx(tx0), .pps(pps), .time_sync(time_sync_r),
        .fe_gpio_in(32'h00000000), .fe_gpio_out(fe_gpio_out32), .fe_gpio_ddr(/* Always assumed to be outputs */),
        .fp_gpio_in({2'b00, fp_gpio_in}), .fp_gpio_out(fp_gpio_out10), .fp_gpio_ddr(fp_gpio_ddr10),
        .bus_clk(bus_clk), .bus_rst(bus_rst),
        .tx_tdata(tx_tdata), .tx_tlast(tx_tlast), .tx_tvalid(tx_tvalid), .tx_tready(tx_tready),
        .rx_tdata(rx_tdata), .rx_tlast(rx_tlast),  .rx_tvalid(rx_tvalid), .rx_tready(rx_tready),
        .ctrl_tdata(r0_ctrl_tdata), .ctrl_tlast(r0_ctrl_tlast),  .ctrl_tvalid(r0_ctrl_tvalid), .ctrl_tready(r0_ctrl_tready),
        .resp_tdata(r0_resp_tdata), .resp_tlast(r0_resp_tlast),  .resp_tvalid(r0_resp_tvalid), .resp_tready(r0_resp_tready),
        .debug()
    );


endmodule // b205_core
