//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//



module u1plus_core
#(
    parameter NUM_RX_DSPS = 2,
    parameter CTRL_ACK_SID = 20,      //needed for reply

    parameter DSP_TX_FIFOSIZE = 10,     //4K MTU
    parameter DSP_RX_FIFOSIZE = 10,     //4K MTU

    parameter DSP_RX_XTRA_FIFOSIZE = 11,
    parameter DSP_TX_XTRA_FIFOSIZE = 11,

    parameter USE_PACKET_PADDER = 0
)
  (input clk, input reset,
   output [31:0] debug, output [1:0] debug_clk,

   // Host Interface
   input [35:0] tx_data, input tx_src_rdy, output tx_dst_rdy,
   output [35:0] rx_data, output rx_src_rdy, input rx_dst_rdy,
   input [35:0] ctrl_data, input ctrl_src_rdy, output ctrl_dst_rdy,
   output [35:0] resp_data, output resp_src_rdy, input resp_dst_rdy,

   output dsp_rx_run, output dsp_tx_run, output clock_sync,

   inout db_sda, inout db_scl,
   output sclk, output [7:0] sen, output mosi, input miso,

   inout [15:0] io_tx, inout [15:0] io_rx,
   output [13:0] tx_i, output [13:0] tx_q,
   input [11:0] rx_i, input [11:0] rx_q,
   input pps_in
   );

    localparam SR_MISC         = 0;      // 5
    localparam SR_USER_REGS    = 5;      // 2
    localparam SR_PADDER       = 10;     // 2

    localparam SR_TX_CTRL      = 32;     // 6
    localparam SR_TX_DSP       = 40;     // 5
    localparam SR_TX_FE        = 48;     // 5

    localparam SR_RX_CTRL0     = 96;      // 9
    localparam SR_RX_DSP0      = 106;     // 7
    localparam SR_RX_FE        = 114;     // 5

    localparam SR_RX_CTRL1     = 128;     // 9
    localparam SR_RX_DSP1      = 138;     // 7

    localparam SR_TIME64       = 192;     // 6
    localparam SR_SPI          = 208;     // 3
    localparam SR_I2C          = 216;     // 1
    localparam SR_GPIO         = 224;     // 5

    //compatibility number -> increment when the fpga has been sufficiently altered
    localparam compat_num = {16'd11, 16'd3}; //major, minor

    //assign run signals used for ATR logic
    wire [NUM_RX_DSPS-1:0] run_rx_n;
    wire run_tx;
    wire run_rx = |(run_rx_n);
    assign dsp_rx_run = run_rx;
    assign dsp_tx_run = run_tx;

    //shared time core signals
    wire [63:0] vita_time, vita_time_pps;

    //shared settings bus signals
    wire set_stb, set_stb_user;
    wire [31:0] set_data, set_data_user;
    wire [7:0] set_addr, set_addr_user;

    //shared SPI core signals
    wire [31:0] spi_readback;
    wire spi_ready;

    //shared I2C core signals
    wire [31:0] i2c_readback;
    wire i2c_ready;

    //shared GPIO core signals
    wire [31:0] gpio_readback;

    ///////////////////////////////////////////////////////////////////////////
    // Misc Registers - persistent across resets
    ///////////////////////////////////////////////////////////////////////////
    wire [31:0] config_word0;
    setting_reg #(.my_addr(SR_MISC+0), .width(32)) sr_misc_config0
     (.clk(clk), .rst(1'b0/*reset*/), .strobe(set_stb), .addr(set_addr), .in(set_data), .out(config_word0));

    wire [31:0] config_word1;
    setting_reg #(.my_addr(SR_MISC+1), .width(32)) sr_misc_config1
     (.clk(clk), .rst(1'b0/*reset*/), .strobe(set_stb), .addr(set_addr), .in(set_data), .out(config_word1));

    wire clock_sync_inv, clock_sync_enb;
    setting_reg #(.my_addr(SR_MISC+2), .width(2)) sr_misc_clock_sync
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
     .out({clock_sync_inv, clock_sync_enb}));

    ///////////////////////////////////////////////////////////////////////////
    // Settings Bus and Readback
    ///////////////////////////////////////////////////////////////////////////
    user_settings #(.BASE(SR_USER_REGS)) user_settings
     (.clk(clk),.rst(reset),
      .set_stb(set_stb), .set_addr(set_addr),.set_data(set_data),
      .set_addr_user(set_addr_user),.set_data_user(set_data_user), .set_stb_user(set_stb_user) );

    wire [35:0] ctrl_out_data, ctrl_int_data;
    wire ctrl_out_src_rdy, ctrl_out_dst_rdy;
    wire ctrl_int_src_rdy, ctrl_int_dst_rdy;

    fifo_cascade #(.WIDTH(36), .SIZE(9)) ctrl_fifo
     (.clk(clk), .reset(reset), .clear(1'b0),
      .datain(ctrl_data), .src_rdy_i(ctrl_src_rdy), .dst_rdy_o(ctrl_dst_rdy), .space(),
      .dataout(ctrl_int_data), .src_rdy_o(ctrl_int_src_rdy), .dst_rdy_i(ctrl_int_dst_rdy), .occupied());

    wire [31:0] num_rx_dsps_rb = NUM_RX_DSPS;

    wire [31:0] sfc_debug;
    settings_fifo_ctrl #(.PROT_HDR(0), .ACK_SID(CTRL_ACK_SID), .XPORT_HDR(0)) sfc
    (
        .clock(clk), .reset(reset), .clear(1'b0),
        .vita_time(vita_time), .perfs_ready(spi_ready & i2c_ready),
        .in_data(ctrl_int_data), .in_valid(ctrl_int_src_rdy), .in_ready(ctrl_int_dst_rdy),
        .out_data(ctrl_out_data), .out_valid(ctrl_out_src_rdy), .out_ready(ctrl_out_dst_rdy),
        .strobe(set_stb), .addr(set_addr), .data(set_data),
        .word00(spi_readback),.word01(compat_num),.word02(i2c_readback),.word03(gpio_readback),
        .word04(config_word0),.word05(config_word1),.word06(num_rx_dsps_rb),.word07(32'hffff_ffff),
        .word08(32'hffff_ffff),.word09(32'hffff_ffff),.word10(vita_time[63:32]),
        .word11(vita_time[31:0]),.word12(32'hffff_ffff),.word13(32'hffff_ffff),
        .word14(vita_time_pps[63:32]),.word15(vita_time_pps[31:0]),
        .debug(sfc_debug)
    );

    ///////////////////////////////////////////////////////////////////////////
    // Time Core
    ///////////////////////////////////////////////////////////////////////////
    time_64bit #(.BASE(SR_TIME64)) time_64bit
     (.clk(clk), .rst(reset), .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .pps(pps_in), .vita_time(vita_time), .vita_time_pps(vita_time_pps),
      .exp_time_in(0));

    assign clock_sync = (clock_sync_enb)? (pps_in ^ clock_sync_inv) : 1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // SPI Core
    ///////////////////////////////////////////////////////////////////////////
    simple_spi_core #(.BASE(SR_SPI), .WIDTH(8), .CLK_IDLE(0), .SEN_IDLE(8'hff))
    simple_spi_core (.clock(clk), .reset(reset),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .readback(spi_readback), .ready(spi_ready),
        .sen(sen), .sclk(sclk), .mosi(mosi), .miso(miso));

    ///////////////////////////////////////////////////////////////////////////
    // I2C Core
    ///////////////////////////////////////////////////////////////////////////
    wire scl_pad_i, scl_pad_o, scl_pad_oen_o, sda_pad_i, sda_pad_o, sda_pad_oen_o;
    simple_i2c_core #(.BASE(SR_I2C)) i2c_core
     (.clock(clk),.reset(reset),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .readback(i2c_readback), .ready(i2c_ready),
      .scl_pad_i(scl_pad_i),.scl_pad_o(scl_pad_o),.scl_padoen_o(scl_pad_oen_o),
      .sda_pad_i(sda_pad_i),.sda_pad_o(sda_pad_o),.sda_padoen_o(sda_pad_oen_o) );

    // I2C -- Don't use external transistors for open drain, the FPGA implements this
    IOBUF scl_pin(.O(scl_pad_i), .IO(db_scl), .I(scl_pad_o), .T(scl_pad_oen_o));
    IOBUF sda_pin(.O(sda_pad_i), .IO(db_sda), .I(sda_pad_o), .T(sda_pad_oen_o));

    ///////////////////////////////////////////////////////////////////////////
    // GPIO Core
    ///////////////////////////////////////////////////////////////////////////
    gpio_atr #(.BASE(SR_GPIO), .WIDTH(32))
    gpio_atr(.clk(clk),.reset(reset),
        .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
        .rx(run_rx), .tx(run_tx), .gpio({io_tx, io_rx}), .gpio_readback(gpio_readback) );

   // /////////////////////////////////////////////////////////////////////////
   // RX ADC Frontend, does IQ Balance, DC Offset, muxing

   wire [23:0] 	 rx_fe_i, rx_fe_q;  // 24 bits is total overkill here, but it matches u2/u2p

   rx_frontend #(.BASE(SR_RX_FE)) rx_frontend
     (.clk(clk),.rst(reset),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .adc_a({rx_i,4'b00}),.adc_ovf_a(0),
      .adc_b({rx_q,4'b00}),.adc_ovf_b(0),
      .i_out(rx_fe_i), .q_out(rx_fe_q), .run(run_rx), .debug());

   // /////////////////////////////////////////////////////////////////////////
   // DSP RX *

    wire [35:0] rx_int2_data [NUM_RX_DSPS-1:0];
    wire rx_int2_src_rdy [NUM_RX_DSPS-1:0];
    wire rx_int2_dst_rdy [NUM_RX_DSPS-1:0];

    genvar dspno;
    generate
    for(dspno = 0; dspno < NUM_RX_DSPS; dspno = dspno + 1) begin:gen_rx_dsps

        wire [31:0] sample_rx;
        wire strobe_rx, clear_rx;
        wire [35:0] vita_rx_data;
        wire vita_rx_src_rdy, vita_rx_dst_rdy;
        wire [35:0] int_rx_data;
        wire int_rx_src_rdy, int_rx_dst_rdy;

       ddc_chain #(.BASE(SR_RX_DSP0+dspno*32), .DSPNO(dspno)) ddc_chain
         (.clk(clk), .rst(reset), .clr(clear_rx),
          .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
          .set_stb_user(set_stb_user), .set_addr_user(set_addr_user), .set_data_user(set_data_user),
          .rx_fe_i(rx_fe_i),.rx_fe_q(rx_fe_q),
          .sample(sample_rx), .run(run_rx_n[dspno]), .strobe(strobe_rx),
          .debug() );

        vita_rx_chain #(.BASE(SR_RX_CTRL0+dspno*32), .UNIT(dspno), .FIFOSIZE(DSP_RX_FIFOSIZE), .PROT_ENG_FLAGS(0), .DSP_NUMBER(dspno)) vita_rx_chain
         (.clk(clk),.reset(reset),
          .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
          .set_stb_user(set_stb_user), .set_addr_user(set_addr_user), .set_data_user(set_data_user),
          .vita_time(vita_time), .overrun(),
          .sample(sample_rx), .run(run_rx_n[dspno]), .strobe(strobe_rx), .clear_o(clear_rx),
          .rx_data_o(vita_rx_data), .rx_dst_rdy_i(vita_rx_dst_rdy), .rx_src_rdy_o(vita_rx_src_rdy),
          .debug() );

        fifo_cascade #(.WIDTH(36), .SIZE(DSP_RX_FIFOSIZE+1)) rx_data_fifo
         (.clk(clk), .reset(reset), .clear(1'b0),
          .datain(vita_rx_data), .src_rdy_i(vita_rx_src_rdy), .dst_rdy_o(vita_rx_dst_rdy), .space(),
          .dataout(int_rx_data), .src_rdy_o(int_rx_src_rdy), .dst_rdy_i(int_rx_dst_rdy), .occupied());

        if (dspno == 0) begin
            assign rx_int2_data[dspno] = int_rx_data;
            assign rx_int2_src_rdy[dspno] = int_rx_src_rdy;
            assign int_rx_dst_rdy = rx_int2_dst_rdy[dspno];
        end
        else begin
            fifo36_mux #(.prio(0)) // No priority, fair sharing
            combine_rx_dsps (
                .clk(clk), .reset(reset), .clear(1'b0/*noclear*/),
                .data0_i(rx_int2_data[dspno-1]), .src0_rdy_i(rx_int2_src_rdy[dspno-1]), .dst0_rdy_o(rx_int2_dst_rdy[dspno-1]),
                .data1_i(int_rx_data), .src1_rdy_i(int_rx_src_rdy), .dst1_rdy_o(int_rx_dst_rdy),
                .data_o(rx_int2_data[dspno]), .src_rdy_o(rx_int2_src_rdy[dspno]), .dst_rdy_i(rx_int2_dst_rdy[dspno])
            );
        end

    end
    endgenerate

   // /////////////////////////////////////////////////////////////////////////
   // RX Stream muxing

    wire [35:0] rx_int3_data;
    wire rx_int3_src_rdy, rx_int3_dst_rdy;

    fifo_cascade #(.WIDTH(36), .SIZE(DSP_RX_XTRA_FIFOSIZE)) rx_data_fifo_combined
         (.clk(clk), .reset(reset), .clear(1'b0),
          .datain(rx_int2_data[NUM_RX_DSPS-1]), .src_rdy_i(rx_int2_src_rdy[NUM_RX_DSPS-1]), .dst_rdy_o(rx_int2_dst_rdy[NUM_RX_DSPS-1]), .space(),
          .dataout(rx_int3_data), .src_rdy_o(rx_int3_src_rdy), .dst_rdy_i(rx_int3_dst_rdy), .occupied());

    generate
    if (USE_PACKET_PADDER) begin
    packet_padder36 #(.BASE(SR_PADDER)) packet_padder_rx_data36(
        .clk(clk), .reset(reset),
        .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
        .data_i(rx_int3_data), .src_rdy_i(rx_int3_src_rdy), .dst_rdy_o(rx_int3_dst_rdy),
        .data_o(rx_data), .src_rdy_o(rx_src_rdy), .dst_rdy_i(rx_dst_rdy),
        .always_flush(~dsp_rx_run));
    end
    else begin
        assign rx_data = rx_int3_data;
        assign rx_src_rdy = rx_int3_src_rdy;
        assign rx_int3_dst_rdy = rx_dst_rdy;
    end
    endgenerate

    ///////////////////////////////////////////////////////////////////////////
    // MUX for TX async and resp data
    ///////////////////////////////////////////////////////////////////////////
    wire [35:0] tx_err_data, resp_data_int;
    wire tx_err_src_rdy, resp_src_rdy_int;
    wire tx_err_dst_rdy, resp_dst_rdy_int;

    fifo36_mux #(.prio(0)) // No priority, fair sharing
    combine_async_and_resp (
        .clk(clk), .reset(reset), .clear(1'b0/*noclear*/),
        .data0_i(ctrl_out_data), .src0_rdy_i(ctrl_out_src_rdy), .dst0_rdy_o(ctrl_out_dst_rdy),
        .data1_i(tx_err_data), .src1_rdy_i(tx_err_src_rdy), .dst1_rdy_o(tx_err_dst_rdy),
        .data_o(resp_data_int), .src_rdy_o(resp_src_rdy_int), .dst_rdy_i(resp_dst_rdy_int)
    );

    fifo_cascade #(.WIDTH(36), .SIZE(9)) resp_fifo
     (.clk(clk), .reset(reset), .clear(1'b0),
      .datain(resp_data_int), .src_rdy_i(resp_src_rdy_int), .dst_rdy_o(resp_dst_rdy_int), .space(),
      .dataout(resp_data), .src_rdy_o(resp_src_rdy), .dst_rdy_i(resp_dst_rdy), .occupied());

   // ///////////////////////////////////////////////////////////////////////////////////
   // DSP TX

   wire [23:0] 	 tx_fe_i, tx_fe_q;
   wire [31:0]   sample_tx;
   wire strobe_tx, clear_tx;

`ifdef DISABLE_TX_DSP
    assign tx_dst_rdy = 1; //null sink
    assign run_tx = 0;
    assign tx_i = 0;
    assign tx_q = 0;
`else
   vita_tx_chain #(.BASE(SR_TX_CTRL),
		   .FIFOSIZE(DSP_TX_FIFOSIZE),
		   .POST_ENGINE_FIFOSIZE(DSP_TX_XTRA_FIFOSIZE),
		   .REPORT_ERROR(1), .DO_FLOW_CONTROL(0),
		   .PROT_ENG_FLAGS(0), .USE_TRANS_HEADER(0),
		   .DSP_NUMBER(0))
   vita_tx_chain
     (.clk(clk), .reset(reset),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .set_stb_user(set_stb_user), .set_addr_user(set_addr_user), .set_data_user(set_data_user),
      .vita_time(vita_time),
      .tx_data_i(tx_data), .tx_src_rdy_i(tx_src_rdy), .tx_dst_rdy_o(tx_dst_rdy),
      .err_data_o(tx_err_data), .err_src_rdy_o(tx_err_src_rdy), .err_dst_rdy_i(tx_err_dst_rdy),
      .sample(sample_tx), .strobe(strobe_tx),
      .underrun(), .run(run_tx), .clear_o(clear_tx),
      .debug());

   duc_chain #(.BASE(SR_TX_DSP), .DSPNO(0)) duc_chain
     (.clk(clk), .rst(reset), .clr(clear_tx),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .set_stb_user(set_stb_user), .set_addr_user(set_addr_user), .set_data_user(set_data_user),
      .tx_fe_i(tx_fe_i),.tx_fe_q(tx_fe_q),
      .sample(sample_tx), .run(run_tx), .strobe(strobe_tx),
      .debug() );

   tx_frontend #(.BASE(SR_TX_FE), .WIDTH_OUT(14)) tx_frontend
     (.clk(clk), .rst(reset),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .tx_i(tx_fe_i), .tx_q(tx_fe_q), .run(1'b1),
      .dac_a(tx_i), .dac_b(tx_q));
`endif
   // /////////////////////////////////////////////////////////////////////////////////////
   // Debug circuitry

   assign debug_clk = 2'b11;
   assign debug = 32'hffffffff;

endmodule // u1plus_core
