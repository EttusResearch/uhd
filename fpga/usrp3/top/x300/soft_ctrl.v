//
// Copyright 2012 Ettus Research LLC
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module soft_ctrl
#(
    parameter SB_ADDRW = 8,
    parameter RB_ADDRW = 8,
    parameter  dw  = 32,  // Data bus width
    parameter  aw  = 16,  // Address bus width, for byte addressibility, 16 = 64K byte memory space
    parameter  sw  = 4   // Select width -- 32-bit data bus with 8-bit granularity.
)
(
    input clk, input rst,
    input clk_div2, input rst_div2,

    //------------------------------------------------------------------
    // I2C interfaces
    //------------------------------------------------------------------
    inout scl0, inout sda0,
    inout scl1, inout sda1,
    inout scl2, inout sda2, 

    //------------------------------------------------------------------
    // UARTs
    //------------------------------------------------------------------
    input gps_rxd,
    output gps_txd,
    input debug_rxd,
    output debug_txd,

    //------------------------------------------------------------------
    // settings bus interface
    //------------------------------------------------------------------
    output [31:0] set_data,
    output [SB_ADDRW-1:0] set_addr,
    output set_stb,

    //------------------------------------------------------------------
    // settings bus interface for crossbar router
    //------------------------------------------------------------------
    output [31:0] set_data_xb,
    output [8:0] set_addr_xb,
    output set_stb_xb,

    //------------------------------------------------------------------
    // readback bus interface
    //------------------------------------------------------------------
    input [31:0] rb_data,
    output [RB_ADDRW-1:0] rb_addr,
    output rb_rd_stb,

    //------------------------------------------------------------------
    // packet interface in
    //------------------------------------------------------------------
    input [63:0] rx_tdata,
    input [3:0] rx_tuser,
    input rx_tlast,
    input rx_tvalid,
    output rx_tready,

    //------------------------------------------------------------------
    // packet interface out
    //------------------------------------------------------------------
    output [63:0] tx_tdata,
    output [3:0] tx_tuser,
    output tx_tlast,
    output tx_tvalid,
    input tx_tready,
 
    //------------------------------------------------------------------
    // PCIe endpoint interface
    //------------------------------------------------------------------
    output [63:0] o_iop2_msg_tdata,
    output o_iop2_msg_tvalid,
    output o_iop2_msg_tlast,
    input o_iop2_msg_tready,
    input [63:0] i_iop2_msg_tdata,
    input i_iop2_msg_tvalid,
    input i_iop2_msg_tlast,
    output i_iop2_msg_tready,
 
    //------------------------------------------------------------------
    // Debug
    //------------------------------------------------------------------
    output [127:0] debug0,
    output [31:0] debug1,

    //------------------------------------------------------------------
    // Wishbone Slave Interface(s)
    //------------------------------------------------------------------
    input [dw-1:0] s4_dat_i,
    output [dw-1:0] s4_dat_o,
    output [aw-1:0] s4_adr,
    output [sw-1:0] s4_sel,
    input s4_ack,
    output s4_stb,
    output s4_cyc,
    output s4_we,
    input s4_int,  // Nothing to connect this too!! No IRQ controller on x300.

    input [dw-1:0] s5_dat_i,
    output [dw-1:0] s5_dat_o,
    output [aw-1:0] s5_adr,
    output [sw-1:0] s5_sel,
    input s5_ack,
    output s5_stb,
    output s5_cyc,
    output s5_we,
    input s5_int  // Nothing to connect this too!! No IRQ controller on x300.


);

    ////////////////////////////////////////////////////////////////////
    // WB interconnect - ZPU, RAM, settings...
    ////////////////////////////////////////////////////////////////////

    wire [dw-1:0] m0_dat_o, m0_dat_i;
    wire [dw-1:0] s0_dat_o, s1_dat_o, s0_dat_i, s1_dat_i, s2_dat_o, s3_dat_o, s2_dat_i, s3_dat_i,
        /* s4_dat_o,s5_dat_o,s4_dat_i,s5_dat_i, */s6_dat_o, s7_dat_o, s6_dat_i, s7_dat_i,
         s8_dat_o, s9_dat_o, s8_dat_i, s9_dat_i, sa_dat_o, sa_dat_i, sb_dat_i, sb_dat_o,
         sc_dat_i, sc_dat_o, sd_dat_i, sd_dat_o, se_dat_i, se_dat_o, sf_dat_i, sf_dat_o;
    wire [aw-1:0] m0_adr,s0_adr,s1_adr,s2_adr,s3_adr,/*s4_adr,s5_adr,*/s6_adr,s7_adr;
    wire [aw-1:0] s8_adr,s9_adr,sa_adr,sb_adr,sc_adr,sd_adr,se_adr,sf_adr;
    wire [sw-1:0] m0_sel,s0_sel,s1_sel,s2_sel,s3_sel,/*s4_sel,s5_sel,*/s6_sel,s7_sel;
    wire [sw-1:0] s8_sel,s9_sel,sa_sel,sb_sel,sc_sel,sd_sel,se_sel,sf_sel;
    wire 	 m0_ack,s0_ack,s1_ack,s2_ack,s3_ack,/*s4_ack,s5_ack,*/s6_ack,s7_ack;
    wire 	 s8_ack,s9_ack,sa_ack,sb_ack,sc_ack,sd_ack,se_ack,sf_ack;
    wire 	 m0_stb,s0_stb,s1_stb,s2_stb,s3_stb,/*s4_stb,s5_stb,*/s6_stb,s7_stb;
    wire 	 s8_stb,s9_stb,sa_stb,sb_stb,sc_stb,sd_stb,se_stb,sf_stb;
    wire 	 m0_cyc,s0_cyc,s1_cyc,s2_cyc,s3_cyc,/*s4_cyc,s5_cyc,*/s6_cyc,s7_cyc;
    wire 	 s8_cyc,s9_cyc,sa_cyc,sb_cyc,sc_cyc,sd_cyc,se_cyc,sf_cyc;
    wire 	 m0_we,s0_we,s1_we,s2_we,s3_we,/*s4_we,s5_we,*/s6_we,s7_we;
    wire 	 s8_we,s9_we,sa_we,sb_we,sc_we,sd_we,se_we,sf_we;

    wb_1master #(.decode_w(8),
        .s0_addr(8'b0000_0000),.s0_mask(8'b1000_0000),  // 0x0000 - Main RAM 32k
        .s1_addr(8'b1000_0000),.s1_mask(8'b1110_0000),  // 0x8000 - PKT RAM 8k
        .s2_addr(8'b1010_0000),.s2_mask(8'b1111_0000),  // 0xa000 - Settings/Readback - misc
        .s3_addr(8'b1011_0000),.s3_mask(8'b1111_0000),  // 0xb000 - Settings/Readback - crossbar
        .s4_addr(8'b1100_0000),.s4_mask(8'b1111_0000),  // 0xc000 - 10GE MAC 0
        .s5_addr(8'b1101_0000),.s5_mask(8'b1111_0000),  // 0xd000 - 10GE MAC 1
        .s6_addr(8'b1111_0110),.s6_mask(8'b1111_1111),  // 0xf600 - Unused
        .s7_addr(8'b1111_0111),.s7_mask(8'b1111_1111),  // 0xf700 - Unused
        .s8_addr(8'b1111_1000),.s8_mask(8'b1111_1111),  // 0xf800 - Unused
        .s9_addr(8'b1111_1001),.s9_mask(8'b1111_1111),  // 0xf900 - UART1 (Debug on GPIO)
        .sa_addr(8'b1111_1010),.sa_mask(8'b1111_1111),  // 0xfa00 - FW Programmer
        .sb_addr(8'b1111_1011),.sb_mask(8'b1111_1111),  // 0xfb00 - PCIe Endpoint
        .sc_addr(8'b1111_1100),.sc_mask(8'b1111_1111),  // 0xfc00 - I2C2
        .sd_addr(8'b1111_1101),.sd_mask(8'b1111_1111),  // 0xfd00 - UART0 (GPS)
        .se_addr(8'b1111_1110),.se_mask(8'b1111_1111),  // 0xfe00 - I2C0
        .sf_addr(8'b1111_1111),.sf_mask(8'b1111_1111),  // 0xff00 - I2C1
        .dw(dw),.aw(aw),.sw(sw))
     wb_1master
       (.clk_i(clk_div2),.rst_i(rst_div2),
    .m0_dat_o(m0_dat_o),.m0_ack_o(m0_ack),.m0_err_o(),.m0_rty_o(),.m0_dat_i(m0_dat_i),
    .m0_adr_i(m0_adr),.m0_sel_i(m0_sel),.m0_we_i(m0_we),.m0_cyc_i(m0_cyc),.m0_stb_i(m0_stb),
    .s0_dat_o(s0_dat_o),.s0_adr_o(s0_adr),.s0_sel_o(s0_sel),.s0_we_o(s0_we),.s0_cyc_o(s0_cyc),.s0_stb_o(s0_stb),
    .s0_dat_i(s0_dat_i),.s0_ack_i(s0_ack),.s0_err_i(1'b0),.s0_rty_i(1'b0),
    .s1_dat_o(s1_dat_o),.s1_adr_o(s1_adr),.s1_sel_o(s1_sel),.s1_we_o(s1_we),.s1_cyc_o(s1_cyc),.s1_stb_o(s1_stb),
    .s1_dat_i(s1_dat_i),.s1_ack_i(s1_ack),.s1_err_i(1'b0),.s1_rty_i(1'b0),
    .s2_dat_o(s2_dat_o),.s2_adr_o(s2_adr),.s2_sel_o(s2_sel),.s2_we_o(s2_we),.s2_cyc_o(s2_cyc),.s2_stb_o(s2_stb),
    .s2_dat_i(s2_dat_i),.s2_ack_i(s2_ack),.s2_err_i(1'b0),.s2_rty_i(1'b0),
    .s3_dat_o(s3_dat_o),.s3_adr_o(s3_adr),.s3_sel_o(s3_sel),.s3_we_o(s3_we),.s3_cyc_o(s3_cyc),.s3_stb_o(s3_stb),
    .s3_dat_i(s3_dat_i),.s3_ack_i(s3_ack),.s3_err_i(1'b0),.s3_rty_i(1'b0),
    .s4_dat_o(s4_dat_o),.s4_adr_o(s4_adr),.s4_sel_o(s4_sel),.s4_we_o(s4_we),.s4_cyc_o(s4_cyc),.s4_stb_o(s4_stb),
    .s4_dat_i(s4_dat_i),.s4_ack_i(s4_ack),.s4_err_i(1'b0),.s4_rty_i(1'b0),
    .s5_dat_o(s5_dat_o),.s5_adr_o(s5_adr),.s5_sel_o(s5_sel),.s5_we_o(s5_we),.s5_cyc_o(s5_cyc),.s5_stb_o(s5_stb),
    .s5_dat_i(s5_dat_i),.s5_ack_i(s5_ack),.s5_err_i(1'b0),.s5_rty_i(1'b0),
    .s6_dat_o(s6_dat_o),.s6_adr_o(s6_adr),.s6_sel_o(s6_sel),.s6_we_o(s6_we),.s6_cyc_o(s6_cyc),.s6_stb_o(s6_stb),
    .s6_dat_i(s6_dat_i),.s6_ack_i(s6_ack),.s6_err_i(1'b0),.s6_rty_i(1'b0),
    .s7_dat_o(s7_dat_o),.s7_adr_o(s7_adr),.s7_sel_o(s7_sel),.s7_we_o(s7_we),.s7_cyc_o(s7_cyc),.s7_stb_o(s7_stb),
    .s7_dat_i(s7_dat_i),.s7_ack_i(s7_ack),.s7_err_i(1'b0),.s7_rty_i(1'b0),
    .s8_dat_o(s8_dat_o),.s8_adr_o(s8_adr),.s8_sel_o(s8_sel),.s8_we_o(s8_we),.s8_cyc_o(s8_cyc),.s8_stb_o(s8_stb),
    .s8_dat_i(s8_dat_i),.s8_ack_i(s8_ack),.s8_err_i(1'b0),.s8_rty_i(1'b0),
    .s9_dat_o(s9_dat_o),.s9_adr_o(s9_adr),.s9_sel_o(s9_sel),.s9_we_o(s9_we),.s9_cyc_o(s9_cyc),.s9_stb_o(s9_stb),
    .s9_dat_i(s9_dat_i),.s9_ack_i(s9_ack),.s9_err_i(1'b0),.s9_rty_i(1'b0),
    .sa_dat_o(sa_dat_o),.sa_adr_o(sa_adr),.sa_sel_o(sa_sel),.sa_we_o(sa_we),.sa_cyc_o(sa_cyc),.sa_stb_o(sa_stb),
    .sa_dat_i(sa_dat_i),.sa_ack_i(sa_ack),.sa_err_i(1'b0),.sa_rty_i(1'b0),
    .sb_dat_o(sb_dat_o),.sb_adr_o(sb_adr),.sb_sel_o(sb_sel),.sb_we_o(sb_we),.sb_cyc_o(sb_cyc),.sb_stb_o(sb_stb),
    .sb_dat_i(sb_dat_i),.sb_ack_i(sb_ack),.sb_err_i(1'b0),.sb_rty_i(1'b0),
    .sc_dat_o(sc_dat_o),.sc_adr_o(sc_adr),.sc_sel_o(sc_sel),.sc_we_o(sc_we),.sc_cyc_o(sc_cyc),.sc_stb_o(sc_stb),
    .sc_dat_i(sc_dat_i),.sc_ack_i(sc_ack),.sc_err_i(1'b0),.sc_rty_i(1'b0),
    .sd_dat_o(sd_dat_o),.sd_adr_o(sd_adr),.sd_sel_o(sd_sel),.sd_we_o(sd_we),.sd_cyc_o(sd_cyc),.sd_stb_o(sd_stb),
    .sd_dat_i(sd_dat_i),.sd_ack_i(sd_ack),.sd_err_i(1'b0),.sd_rty_i(1'b0),
    .se_dat_o(se_dat_o),.se_adr_o(se_adr),.se_sel_o(se_sel),.se_we_o(se_we),.se_cyc_o(se_cyc),.se_stb_o(se_stb),
    .se_dat_i(se_dat_i),.se_ack_i(se_ack),.se_err_i(1'b0),.se_rty_i(1'b0),
    .sf_dat_o(sf_dat_o),.sf_adr_o(sf_adr),.sf_sel_o(sf_sel),.sf_we_o(sf_we),.sf_cyc_o(sf_cyc),.sf_stb_o(sf_stb),
    .sf_dat_i(sf_dat_i),.sf_ack_i(sf_ack),.sf_err_i(1'b0),.sf_rty_i(1'b0) );

    //assign {s0_dat_i, s0_ack} = 33'b0;
    //assign {s1_dat_i, s1_ack} = 33'b0;
    //assign {s2_dat_i, s2_ack} = 33'b0;
    //assign {s3_dat_i, s3_ack} = 33'b0;
    //assign {s4_dat_i, s4_ack} = 33'b0;
    //assign {s5_dat_i, s5_ack} = 33'b0;
    assign {s6_dat_i, s6_ack} = 33'b0;
    assign {s7_dat_i, s7_ack} = 33'b0;
    assign {s8_dat_i, s8_ack} = 33'b0;
    //assign {s9_dat_i, s9_ack} = 33'b0;
    //assign {sa_dat_i, sa_ack} = 33'b0;
    assign sa_dat_i = 32'b0;
    //assign {sb_dat_i, sb_ack} = 33'b0;
    //assign {sc_dat_i, sc_ack} = 33'b0;
    //assign {sd_dat_i, sd_ack} = 33'b0;
    //assign {se_dat_i, se_ack} = 33'b0;
    //assign {sf_dat_i, sf_ack} = 33'b0;


    ////////////////////////////////////////////////////////////////////
    // Processor
    ////////////////////////////////////////////////////////////////////
    wire zpu_rst;

    zpu_wb_top #(.dat_w(dw), .adr_w(aw), .sel_w(sw))
     zpu_top0 (.clk(clk_div2), .rst(zpu_rst), .enb(~zpu_rst),
       // Data Wishbone bus to system bus fabric
       .we_o(m0_we),.stb_o(m0_stb),.dat_o(m0_dat_i),.adr_o(m0_adr),
       .dat_i(m0_dat_o),.ack_i(m0_ack),.sel_o(m0_sel),.cyc_o(m0_cyc),
       // Interrupts and exceptions
       .zpu_status(), .interrupt(1'b0));

    //assign {debug1, debug0} = {bank_swap, zpu_rst, swap_addr, s0_we, s0_stb, s3_ack, s2_ack, s1_ack, s0_ack, txd, clk, rst, m0_we, m0_stb, m0_ack, m0_cyc, m0_adr, m0_dat_i};

    ////////////////////////////////////////////////////////////////////
    // Double buffered system RAM (Slave #0) and Bootloader (Slave #A)
    ////////////////////////////////////////////////////////////////////
    zpu_bootram #(.ADDR_WIDTH(aw), .DATA_WIDTH(dw), .MAX_ADDR(16'h7FFC)) sys_ram 
    (
        .clk(clk_div2), .rst(rst_div2),
        .mem_stb(s0_stb), .mem_wea(&({4{s0_we}} & s0_sel)), .mem_acka(s0_ack),
        .mem_addra(s0_adr), .mem_dina(s0_dat_o), .mem_douta(s0_dat_i),
        .ldr_stb(sa_stb), .ldr_wea(&({4{sa_we}} & sa_sel)),
        .ldr_addra(sa_adr), .ldr_dina(sa_dat_o), .ldr_acka(sa_ack),
        .zpu_rst(zpu_rst)
    );

    ////////////////////////////////////////////////////////////////////
    // Packet RAM -- Slave #1
    ////////////////////////////////////////////////////////////////////

    //------------------------------------------------------------------
    // packet interface in div2
    //------------------------------------------------------------------
    wire [63:0] rx_tdata_div2;
    wire [3:0] rx_tuser_div2;
    wire rx_tlast_div2;
    wire rx_tvalid_div2;
    wire rx_tready_div2;

    //------------------------------------------------------------------
    // packet interface out div2
    //------------------------------------------------------------------
    wire [63:0] tx_tdata_div2;
    wire [3:0] tx_tuser_div2;
    wire tx_tlast_div2;
    wire tx_tvalid_div2;
    wire tx_tready_div2; 

    //clock cross fifo between bus_clk and bus_clk_div2 for axi stream input
    //WIDTH = tdata+tuser+tlast = 69  
    axi_fifo_2clk #(.WIDTH(69), .SIZE(5)) axi_stream_rx_fifo_2clk
    (.reset(rst),
      .i_aclk(clk), .i_tdata({rx_tdata, rx_tuser, rx_tlast}), .i_tvalid(rx_tvalid), .i_tready(rx_tready),
      .o_aclk(clk_div2), .o_tdata({rx_tdata_div2, rx_tuser_div2, rx_tlast_div2}), .o_tvalid(rx_tvalid_div2), .o_tready(rx_tready_div2));
    
    //clock cross fifo between bus_clk_div2 and bus_clk for axi stream output
    //WIDTH = tdata+tuser+tlast = 69  
    axi_fifo_2clk #(.WIDTH(69), .SIZE(5)) axi_stream_tx_fifo_2clk
    (.reset(reset),
      .i_aclk(clk_div2), .i_tdata({tx_tdata_div2, tx_tuser_div2, tx_tlast_div2}), .i_tvalid(tx_tvalid_div2), .i_tready(tx_tready_div2),
      .o_aclk(clk), .o_tdata({tx_tdata, tx_tuser, tx_tlast}), .o_tvalid(tx_tvalid), .o_tready(tx_tready));
    
    axi_stream_to_wb #(.AWIDTH(13), .CTRL_ADDR(13'h1ffc)) axi_stream_to_wb
    (
        .clk_i(clk_div2), .rst_i(rst_div2),

        //wb interface
        .we_i(s1_we), .stb_i(s1_stb), .cyc_i(s1_cyc), .ack_o(s1_ack),
        .adr_i(s1_adr[12:0]), .dat_i(s1_dat_o), .dat_o(s1_dat_i),

        //axi stream in
        .rx_tdata(rx_tdata_div2), .rx_tuser(rx_tuser_div2), .rx_tlast(rx_tlast_div2),
        .rx_tvalid(rx_tvalid_div2), .rx_tready(rx_tready_div2),

        //axi stream out
        .tx_tdata(tx_tdata_div2), .tx_tuser(tx_tuser_div2), .tx_tlast(tx_tlast_div2),
        .tx_tvalid(tx_tvalid_div2), .tx_tready(tx_tready_div2),

        .debug_rx(), .debug_tx()
    );



    ////////////////////////////////////////////////////////////////////
    // Settings and readback bus -- Slave #2
    ////////////////////////////////////////////////////////////////////
    settings_bus #(.AWIDTH(aw), .DWIDTH(dw), .SWIDTH(SB_ADDRW)) settings_bus
    (
        .wb_clk(clk_div2), .wb_rst(rst_div2),
        .wb_adr_i(s2_adr), .wb_dat_i(s2_dat_o),
        .wb_stb_i(s2_stb), .wb_we_i(s2_we), .wb_ack_o(s2_ack),
        .strobe(set_stb), .addr(set_addr), .data(set_data)
    );

    settings_readback #(.AWIDTH(aw),.DWIDTH(dw), .RB_ADDRW(RB_ADDRW)) settings_readback
    (
        .wb_clk(clk_div2), 
        .wb_rst(rst_div2), 
        .wb_adr_i(s2_adr),
        .wb_stb_i(s2_stb),
        .wb_we_i(s2_we),
        .rb_data(rb_data),
        .rb_addr(rb_addr),
        .wb_dat_o(s2_dat_i),
        .rb_rd_stb(rb_rd_stb)
    );

   // assign rb_addr = s2_adr[RB_ADDRW+1:2];
   // assign s2_dat_i = rb_data;

    ////////////////////////////////////////////////////////////////////
    // Settings bus for cross bar -- Slave #3
    ////////////////////////////////////////////////////////////////////
    settings_bus #(.AWIDTH(aw), .DWIDTH(dw), .SWIDTH(9)) settings_bus_xb
    (
        .wb_clk(clk), .wb_rst(rst),
        .wb_adr_i(s3_adr), .wb_dat_i(s3_dat_o),
        .wb_stb_i(s3_stb), .wb_we_i(s3_we), .wb_ack_o(s3_ack),
        .strobe(set_stb_xb), .addr(set_addr_xb), .data(set_data_xb)
    );

    assign s3_dat_i = 32'b0;

    ////////////////////////////////////////////////////////////////////
    // 10GE MAC 0 -- Slave #4
    ////////////////////////////////////////////////////////////////////
    // External to this Heirarchy.

    ////////////////////////////////////////////////////////////////////
    // 10GE MAC 1 -- Slave #5
    ////////////////////////////////////////////////////////////////////
    // External to this Heirarchy.


    ////////////////////////////////////////////////////////////////////
    // UART0 -- Slave #9
    ////////////////////////////////////////////////////////////////////
    simple_uart zpu_debug_uart
    (
        .clk_i(clk_div2), .rst_i(rst_div2),
        .we_i(s9_we), .stb_i(s9_stb), .cyc_i(s9_cyc), .ack_o(s9_ack),
        .adr_i(s9_adr[4:2]), .dat_i(s9_dat_o), .dat_o(s9_dat_i),
        .rx_int_o(), .tx_int_o(), .tx_o(debug_txd), .rx_i(debug_rxd), .baud_o()
    );
   
    ////////////////////////////////////////////////////////////////////
    // PCIe endpoint -- Slave #b
    ////////////////////////////////////////////////////////////////////
    
    //------------------------------------------------------------------
    // PCIe endpoint interface in div2
    //------------------------------------------------------------------
    wire [63:0] o_iop2_msg_tdata_div2;
    wire o_iop2_msg_tvalid_div2;
    wire o_iop2_msg_tlast_div2;
    wire o_iop2_msg_tready_div2;
    wire [63:0] i_iop2_msg_tdata_div2;
    wire i_iop2_msg_tvalid_div2;
    wire i_iop2_msg_tlast_div2;
    wire i_iop2_msg_tready_div2;


    //clock cross fifo between bus_clk and bus_clk_div2 for i_iop2_msg 
    //WIDTH = tdata+tuser+tlast = 69  
    axi_fifo_2clk #(.WIDTH(65), .SIZE(5)) i_iop2_msg_fifo_2clk
    (.reset(reset),
      .i_aclk(clk), .i_tdata({i_iop2_msg_tdata, i_iop2_msg_tlast}), .i_tvalid(i_iop2_msg_tvalid), .i_tready(i_iop2_msg_tready),
      .o_aclk(clk_div2), .o_tdata({i_iop2_msg_tdata_div2, iop2_msg_tlast_div2}), .o_tvalid(i_iop2_msg_tvalid_div2), .o_tready(i_iop2_msg_tready_div2));
    
    //clock cross fifo between bus_clk_div2 and bus_clk for o_iop2_msg
    //WIDTH = tdata+tuser+tlast = 69  
    axi_fifo_2clk #(.WIDTH(65), .SIZE(5)) o_iop2_msg_fifo_2clk
    (.reset(reset),
      .i_aclk(clk_div2), .i_tdata({o_iop2_msg_tdata_div2, 1'b1}), .i_tvalid(o_iop2_msg_tvalid_div2), .i_tready(o_iop2_msg_tready_div2),
      .o_aclk(clk), .o_tdata({o_iop2_msg_tdata, o_iop2_msg_tlast}), .o_tvalid(o_iop2_msg_tvalid), .o_tready(o_iop2_msg_tready));
    
    pcie_wb_reg_core #(.WB_ADDRW(aw), .WB_DATAW(dw)) pcie_reg_core 
    (
        .clk(clk_div2), .rst(rst_div2),
        .wb_stb_i(sb_stb), .wb_we_i(sb_we), .wb_adr_i(sb_adr),
        .wb_dat_i(sb_dat_o), .wb_ack_o(sb_ack), .wb_dat_o(sb_dat_i),
        .msgi_tdata(i_iop2_msg_tdata_div2), .msgi_tvalid(i_iop2_msg_tvalid_div2), .msgi_tready(i_iop2_msg_tready_div2),
        .msgo_tdata(o_iop2_msg_tdata_div2), .msgo_tvalid(o_iop2_msg_tvalid_div2), .msgo_tready(o_iop2_msg_tready_div2),
        .debug(debug0)
    );

    ////////////////////////////////////////////////////////////////////
    // I2C2 -- Slave #c
    ////////////////////////////////////////////////////////////////////
    wire scl2_pad_i, scl2_pad_o, scl2_pad_oen_o;
    wire sda2_pad_i, sda2_pad_o, sda2_pad_oen_o;

    i2c_master_top #(.ARST_LVL(1)) i2c2
    (
        .wb_clk_i(clk_div2),.wb_rst_i(rst_div2),.arst_i(1'b0),
        .wb_adr_i(sc_adr[4:2]),.wb_dat_i(sc_dat_o[7:0]),.wb_dat_o(sc_dat_i[7:0]),
        .wb_we_i(sc_we),.wb_stb_i(sc_stb),.wb_cyc_i(sc_cyc),
        .wb_ack_o(sc_ack),.wb_inta_o(),
        .scl_pad_i(scl2_pad_i),.scl_pad_o(scl2_pad_o),.scl_padoen_o(scl2_pad_oen_o),
        .sda_pad_i(sda2_pad_i),.sda_pad_o(sda2_pad_o),.sda_padoen_o(sda2_pad_oen_o)
    );

    // I2C -- Don't use external transistors for open drain, the FPGA implements this
    IOBUF scl2_pin(.O(scl2_pad_i), .IO(scl2), .I(scl2_pad_o), .T(scl2_pad_oen_o));
    IOBUF sda2_pin(.O(sda2_pad_i), .IO(sda2), .I(sda2_pad_o), .T(sda2_pad_oen_o));

    assign sc_dat_i[31:8] = 24'd0;

 
    ////////////////////////////////////////////////////////////////////
    // UART0 -- Slave #d
    ////////////////////////////////////////////////////////////////////
    simple_uart gps_uart
    (
        .clk_i(clk_div2), .rst_i(rst_div2),
        .we_i(sd_we), .stb_i(sd_stb), .cyc_i(sd_cyc), .ack_o(sd_ack),
        .adr_i(sd_adr[4:2]), .dat_i(sd_dat_o), .dat_o(sd_dat_i),
        .rx_int_o(), .tx_int_o(), .tx_o(gps_txd), .rx_i(gps_rxd), .baud_o()
    );

    ////////////////////////////////////////////////////////////////////
    // I2C0 -- Slave #e
    ////////////////////////////////////////////////////////////////////
    wire scl0_pad_i, scl0_pad_o, scl0_pad_oen_o;
    wire sda0_pad_i, sda0_pad_o, sda0_pad_oen_o;

    i2c_master_top #(.ARST_LVL(1)) i2c0
    (
        .wb_clk_i(clk_div2),.wb_rst_i(rst_div2),.arst_i(1'b0),
        .wb_adr_i(se_adr[4:2]),.wb_dat_i(se_dat_o[7:0]),.wb_dat_o(se_dat_i[7:0]),
        .wb_we_i(se_we),.wb_stb_i(se_stb),.wb_cyc_i(se_cyc),
        .wb_ack_o(se_ack),.wb_inta_o(),
        .scl_pad_i(scl0_pad_i),.scl_pad_o(scl0_pad_o),.scl_padoen_o(scl0_pad_oen_o),
        .sda_pad_i(sda0_pad_i),.sda_pad_o(sda0_pad_o),.sda_padoen_o(sda0_pad_oen_o)
    );

    // I2C -- Don't use external transistors for open drain, the FPGA implements this
    IOBUF scl0_pin(.O(scl0_pad_i), .IO(scl0), .I(scl0_pad_o), .T(scl0_pad_oen_o));
    IOBUF sda0_pin(.O(sda0_pad_i), .IO(sda0), .I(sda0_pad_o), .T(sda0_pad_oen_o));

    assign se_dat_i[31:8] = 24'd0;

    ////////////////////////////////////////////////////////////////////
    // I2C1 -- Slave #f
    ////////////////////////////////////////////////////////////////////
    wire scl1_pad_i, scl1_pad_o, scl1_pad_oen_o;
    wire sda1_pad_i, sda1_pad_o, sda1_pad_oen_o;

    i2c_master_top #(.ARST_LVL(1)) i2c1
    (
        .wb_clk_i(clk_div2),.wb_rst_i(rst_div2),.arst_i(1'b0),
        .wb_adr_i(sf_adr[4:2]),.wb_dat_i(sf_dat_o[7:0]),.wb_dat_o(sf_dat_i[7:0]),
        .wb_we_i(sf_we),.wb_stb_i(sf_stb),.wb_cyc_i(sf_cyc),
        .wb_ack_o(sf_ack),.wb_inta_o(),
        .scl_pad_i(scl1_pad_i),.scl_pad_o(scl1_pad_o),.scl_padoen_o(scl1_pad_oen_o),
        .sda_pad_i(sda1_pad_i),.sda_pad_o(sda1_pad_o),.sda_padoen_o(sda1_pad_oen_o)
    );

    // I2C -- Don't use external transistors for open drain, the FPGA implements this
    IOBUF scl1_pin(.O(scl1_pad_i), .IO(scl1), .I(scl1_pad_o), .T(scl1_pad_oen_o));
    IOBUF sda1_pin(.O(sda1_pad_i), .IO(sda1), .I(sda1_pad_o), .T(sda1_pad_oen_o));

    assign sf_dat_i[31:8] = 24'd0;



endmodule //soft_ctrl
