//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


// radio top level module for b200
//  Contains all clock-rate DSP components, all radio and hardware controls and settings

module radio_legacy
  #(
    parameter RADIO_FIFO_SIZE = 13,
    parameter SAMPLE_FIFO_SIZE = 11,
    parameter FP_GPIO = 0,
    parameter NEW_HB_INTERP = 0,
    parameter NEW_HB_DECIM = 0,
    parameter SOURCE_FLOW_CONTROL = 0,
    parameter USER_SETTINGS = 0,
    parameter DEVICE = "SPARTAN6"
  )
  (input radio_clk, input radio_rst,
   input [31:0] rx, output reg [31:0] tx,
   input [31:0] fe_gpio_in, output [31:0] fe_gpio_out, output [31:0] fe_gpio_ddr,
   input [9:0] fp_gpio_in, output [9:0] fp_gpio_out, output [9:0] fp_gpio_ddr,
   input pps, input time_sync,
   input bus_clk, input bus_rst,
   input [63:0]  tx_tdata, input tx_tlast, input tx_tvalid, output tx_tready,
   output [63:0] rx_tdata, output rx_tlast, output rx_tvalid, input rx_tready,
   input [63:0]  ctrl_tdata, input ctrl_tlast, input ctrl_tvalid, output ctrl_tready,
   output [63:0] resp_tdata, output resp_tlast, output resp_tvalid, input resp_tready,

   output reg [63:0] vita_time_b,

   output [63:0] debug
   );


   // ///////////////////////////////////////////////////////////////////////////////
   // FIFO Interfacing to the bus clk domain
   // in_tdata splits to tx_tdata and ctrl_tdata
   // rx_tdata and resp_tdata get muxed to out_tdata
   // Everything except rx flow control must cross in to radio_clk domain before further use
   // _b signifies bus_clk domain, _r signifies radio_clk domain

   wire [63:0] 	 ctrl_tdata_r;
   wire 	 ctrl_tready_r, ctrl_tvalid_r;
   wire 	 ctrl_tlast_r;

   wire [63:0] 	 resp_tdata_r;
   wire 	 resp_tready_r, resp_tvalid_r;
   wire 	 resp_tlast_r;

   wire [63:0] 	 rx_tdata_r;
   wire 	 rx_tready_r, rx_tvalid_r;
   wire 	 rx_tlast_r;

   wire [63:0] 	 rx_err_tdata_r;
   wire 	 rx_err_tready_r, rx_err_tvalid_r;
   wire 	 rx_err_tlast_r;

   wire [63:0]     rx_prefc_tdata_r;
   wire   rx_prefc_tready_r, rx_prefc_tvalid_r;
   wire   rx_prefc_tlast_r;

   wire [63:0]     rx_postfc_tdata_r;
   wire   rx_postfc_tready_r, rx_postfc_tvalid_r;
   wire   rx_postfc_tlast_r;

   wire [63:0] 	 tx_tdata_r;
   wire 	 tx_tready_r, tx_tvalid_r;
   wire 	 tx_tlast_r;

   wire [63:0] 	 txresp_tdata, txresp_tdata_r;
   wire 	 txresp_tready, txresp_tready_r, txresp_tvalid, txresp_tvalid_r;
   wire 	 txresp_tlast, txresp_tlast_r;

   wire [63:0] 	 rmux_tdata_r;
   wire 	 rmux_tlast_r, rmux_tvalid_r, rmux_tready_r;

   wire [31:0] 	 tx_idle;
   wire [3:0] 	 ibs_state;
   wire [63:0] 	 rx_tdata_int;
   wire 	 rx_tready_int, rx_tvalid_int;
   wire 	 rx_tlast_int;


   axi_fifo_2clk #(.WIDTH(65), .SIZE(0/*minimal*/)) ctrl_fifo
     (.reset(bus_rst),
      .i_aclk(bus_clk), .i_tvalid(ctrl_tvalid), .i_tready(ctrl_tready), .i_tdata({ctrl_tlast, ctrl_tdata}),
      .o_aclk(radio_clk), .o_tvalid(ctrl_tvalid_r), .o_tready(ctrl_tready_r), .o_tdata({ctrl_tlast_r, ctrl_tdata_r}));

   axi_fifo_2clk #(.WIDTH(65), .SIZE(RADIO_FIFO_SIZE)) tx_fifo
     (.reset(bus_rst),
      .i_aclk(bus_clk), .i_tvalid(tx_tvalid), .i_tready(tx_tready), .i_tdata({tx_tlast, tx_tdata}),
      .o_aclk(radio_clk), .o_tvalid(tx_tvalid_r), .o_tready(tx_tready_r), .o_tdata({tx_tlast_r, tx_tdata_r}));

   axi_fifo_2clk #(.WIDTH(65), .SIZE(0/*minimal*/)) resp_fifo
     (.reset(radio_rst),
      .i_aclk(radio_clk), .i_tvalid(rmux_tvalid_r), .i_tready(rmux_tready_r), .i_tdata({rmux_tlast_r, rmux_tdata_r}),
      .o_aclk(bus_clk), .o_tvalid(resp_tvalid), .o_tready(resp_tready), .o_tdata({resp_tlast, resp_tdata}));

   axi_fifo_2clk #(.WIDTH(65), .SIZE(RADIO_FIFO_SIZE)) rx_fifo
     (.reset(radio_rst),
      .i_aclk(radio_clk), .i_tvalid(rx_tvalid_r), .i_tready(rx_tready_r), .i_tdata({rx_tlast_r, rx_tdata_r}),
      .o_aclk(bus_clk), .o_tvalid(rx_tvalid_int), .o_tready(rx_tready_int), .o_tdata({rx_tlast_int, rx_tdata_int}));

   axi_packet_gate #(.WIDTH(64), .SIZE(SAMPLE_FIFO_SIZE), .USE_AS_BUFF(0)) buffer_whole_pkt
     (
      .clk(bus_clk), .reset(bus_rst), .clear(1'b0),
      .i_tdata(rx_tdata_int), .i_tlast(rx_tlast_int), .i_terror(1'b0), .i_tvalid(rx_tvalid_int), .i_tready(rx_tready_int),
      .o_tdata(rx_tdata), .o_tlast(rx_tlast), .o_tvalid(rx_tvalid), .o_tready(rx_tready));

   ///////////////////////////////////////////////////////////////////////////////////////
   // Setting bus and controls

   wire [63:0]    ctrl_tdata_proc;
   wire           ctrl_tready_proc, ctrl_tvalid_proc;
   wire           ctrl_tlast_proc;

   localparam SR_LOOPBACK     = 8'd6;
   localparam SR_SPI          = 8'd8;
   localparam SR_ATR          = 8'd12; // thorugh 8'd18
   localparam SR_TEST         = 8'd21;
   localparam SR_CODEC_IDLE   = 8'd22;
   localparam SR_READBACK     = 8'd32;
   localparam SR_TX_CTRL      = 8'd64;
   localparam SR_RX_CTRL      = 8'd96;
   localparam SR_TIME         = 8'd128;
   localparam SR_RX_FMT       = 8'd136;
   localparam SR_TX_FMT       = 8'd138;
   localparam SR_RX_DSP       = 8'd144;
   localparam SR_TX_DSP       = 8'd184;
   localparam SR_FP_GPIO      = 8'd200; // thorugh 8'd206
   localparam SR_USER_SR_BASE = 8'd253;
   localparam SR_USER_RB_ADDR = 8'd255;

   wire           set_stb;
   wire [7:0]     set_addr;
   wire [31:0]    set_data;
   wire [31:0]    test_readback;
   wire [9:0] 	  fp_gpio_readback;
   wire           run_rx, run_tx;
   wire           rx_flow_ctrl_busy;

   reg [63:0]     rb_data;
   wire [2:0]     rb_addr;

   wire [63:0] vita_time, vita_time_lastpps;
   timekeeper_legacy #(.SR_TIME_HI(SR_TIME), .SR_TIME_LO(SR_TIME+1), .SR_TIME_CTRL(SR_TIME+2)) timekeeper
     (.clk(radio_clk), .reset(radio_rst), .pps(pps), .sync_in(time_sync), .strobe(1'b1),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time), .vita_time_lastpps(vita_time_lastpps),
      .sync_out());

   wire [31:0] debug_radio_ctrl_proc;
   radio_ctrl_proc radio_ctrl_proc
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .ctrl_tdata(ctrl_tdata_proc), .ctrl_tlast(ctrl_tlast_proc), .ctrl_tvalid(ctrl_tvalid_proc), .ctrl_tready(ctrl_tready_proc),
      .resp_tdata(resp_tdata_r), .resp_tlast(resp_tlast_r), .resp_tvalid(resp_tvalid_r), .resp_tready(resp_tready_r),
      .vita_time(vita_time),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .ready(1'b1), .readback(rb_data),
      .debug(debug_radio_ctrl_proc));

   reg [63:0]     rb_data_user;
generate
   if (USER_SETTINGS == 1) begin
      wire           set_stb_user;
      wire [7:0]     set_addr_user;
      wire [31:0]    set_data_user;
      wire [7:0]     rb_addr_user;

      user_settings #(.BASE(SR_USER_SR_BASE)) user_settings
        (.clk(radio_clk), .rst(radio_rst),
         .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
         .set_stb_user(set_stb_user), .set_addr_user(set_addr_user), .set_data_user(set_data_user));

      setting_reg #(.my_addr(SR_USER_RB_ADDR), .awidth(8), .width(8)) user_rb_addr
        (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
         .out(rb_addr_user), .changed());

      // ----------------------------------
      // Enter user settings registers here
      // ----------------------------------

      // Example code for 32-bit settings registers and 64-bit readback registers
      //
      // To test this, modify the *_core.v file for your specific USRP and set
      // USER_SETTINGS=1 for the parameters for the radio_legacy instantiation.
      //
      // You can then use the get_user_settings_iface() like this:
      //
      // auto usrp = multi_usrp::make("type=b200,enable_user_regs");
      // auto regs = usrp->get_user_settings_iface(0);
      // regs->poke32(0, 0xCAFE);
      // regs->poke32(4, 0xBEEF);
      // std::cout << boost::format("0x%016X") % regs->peek64(0) << std::endl;
      wire [31:0] user_reg_0_value, user_reg_1_value;

      setting_reg #(.my_addr(8'd0), .awidth(8), .width(32)) user_reg_0
        (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb_user), .addr(set_addr_user), .in(set_data_user),
         .out(user_reg_0_value), .changed());

      setting_reg #(.my_addr(8'd1), .awidth(8), .width(32)) user_reg_1
        (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb_user), .addr(set_addr_user), .in(set_data_user),
         .out(user_reg_1_value), .changed());

      always @* begin
         case(rb_addr_user)
            8'd0 : rb_data_user <= {user_reg_1_value, user_reg_0_value};
            default : rb_data_user <= 64'd0;
         endcase
      end

   end else begin    //for USER_SETTINGS == 1
      always @* rb_data_user <= 64'd0;
   end
endgenerate

   always @*
     case(rb_addr)
       3'd0 : rb_data <= { 32'b0, test_readback };
       3'd1 : rb_data <= vita_time;
       3'd2 : rb_data <= vita_time_lastpps;
       3'd3 : rb_data <= {tx, rx};
       3'd4 : rb_data <= {54'h0,fp_gpio_readback};
       3'd5 : rb_data <= {59'h0,rx_flow_ctrl_busy,ibs_state[3:0]}; // Monitor state of RX state machine.
//     3'd6 : rb_data <= <unused>;
       3'd7 : rb_data <= rb_data_user;
       default : rb_data <= 64'd0;
     endcase // case (rb_addr)

   //
   // Sample VITA_TIME into the bus_clk domain for use by instrumentation.
   //
   wire [63:0] vita_time_b_int;
   wire        vita_time_b_valid;

    axi_fifo_2clk #(.WIDTH(64), .SIZE(0)) vita_time_fifo
     (.reset(radio_rst),
      .i_aclk(radio_clk), .i_tvalid(1'b1), .i_tready(), .i_tdata(vita_time),
      .o_aclk(bus_clk), .o_tvalid(vita_time_b_valid), .o_tready(1'b1), .o_tdata(vita_time_b_int));

   always @(posedge bus_clk)
     if (vita_time_b_valid)
       vita_time_b <= vita_time_b_int;

   // Set this register to loop TX data directly to RX data.
   setting_reg #(.my_addr(SR_LOOPBACK), .awidth(8), .width(1)) sr_loopback
     (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(loopback), .changed());

   setting_reg #(.my_addr(SR_TEST), .awidth(8), .width(32)) sr_test
     (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(test_readback), .changed());

   setting_reg #(.my_addr(SR_CODEC_IDLE), .awidth(8), .width(32)) sr_codec_idle
     (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(tx_idle), .changed());

   setting_reg #(.my_addr(SR_READBACK), .awidth(8), .width(3)) sr_rdback
     (.clk(radio_clk), .rst(radio_rst), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(rb_addr), .changed());

   //The fe_atr pins driven by this module are always configured as outputs so default
   //the DDR (data direction register) to be all ones (outputs) so that the drive direction
   //these lines does not change during/after resets.
   gpio_atr #(.BASE(SR_ATR), .WIDTH(32), .FAB_CTRL_EN(0), .DEFAULT_DDR(32'hFFFFFFFF), .DEFAULT_IDLE(32'h00000000)) fe_gpio_atr
     (.clk(radio_clk),.reset(radio_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .rx(run_rx), .tx(run_tx),
      .gpio_in(fe_gpio_in), .gpio_out(fe_gpio_out), .gpio_ddr(fe_gpio_ddr),
      .gpio_out_fab(32'h00000000 /* no fabric control */), .gpio_sw_rb() );

   generate
      if (FP_GPIO != 0) begin: add_fp_gpio
         gpio_atr #(.BASE(SR_FP_GPIO), .WIDTH(10), .FAB_CTRL_EN(0)) fp_gpio_atr
            (.clk(radio_clk),.reset(radio_rst),
            .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
            .rx(run_rx), .tx(run_tx),
            .gpio_in(fp_gpio_in), .gpio_out(fp_gpio_out), .gpio_ddr(fp_gpio_ddr), 
            .gpio_out_fab(10'h000 /* no fabric control */), .gpio_sw_rb(fp_gpio_readback));
      end
   endgenerate



   ///////////////////////////////////////////////////////////////////////////////////////
   // Source flow control

generate
   if (SOURCE_FLOW_CONTROL == 1) begin

      localparam SID_PREFIX_CTRL = 2'd0;
      localparam SID_PREFIX_FC   = 2'd1;

      wire [63:0]    ctrl_tdata_fc;
      wire           ctrl_tready_fc, ctrl_tvalid_fc;
      wire           ctrl_tlast_fc;

      wire [63:0]    ctrl_hdr;
      wire [1:0]     ctrl_dest;

      assign ctrl_dest = (ctrl_hdr[1:0] == SID_PREFIX_FC) ? 2'd1 : 2'd0;

      axi_demux4 #(.ACTIVE_CHAN(4'b0011), .WIDTH(64), .BUFFER(1)) demux_proc_fc
        (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
         .header(ctrl_hdr), .dest(ctrl_dest),
         .i_tdata(ctrl_tdata_r), .i_tlast(ctrl_tlast_r), .i_tvalid(ctrl_tvalid_r), .i_tready(ctrl_tready_r),                  //Input
         .o0_tdata(ctrl_tdata_proc), .o0_tlast(ctrl_tlast_proc), .o0_tvalid(ctrl_tvalid_proc), .o0_tready(ctrl_tready_proc),  //Settings/Readback
         .o1_tdata(ctrl_tdata_fc), .o1_tlast(ctrl_tlast_fc), .o1_tvalid(ctrl_tvalid_fc), .o1_tready(ctrl_tready_fc),          //Flow control
         .o2_tdata(), .o2_tlast(), .o2_tvalid(), .o2_tready(1'b0),                                                            //Unused
         .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b0));                                                           //Unused

      source_flow_control_legacy #(.BASE(SR_RX_CTRL+6)) rx_sfc
        (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
         .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
         .fc_tdata(ctrl_tdata_fc), .fc_tlast(ctrl_tlast_fc), .fc_tvalid(ctrl_tvalid_fc), .fc_tready(ctrl_tready_fc),                      //Flow control In
         .in_tdata(rx_prefc_tdata_r), .in_tlast(rx_prefc_tlast_r), .in_tvalid(rx_prefc_tvalid_r), .in_tready(rx_prefc_tready_r),          //RX Input
         .out_tdata(rx_postfc_tdata_r), .out_tlast(rx_postfc_tlast_r), .out_tvalid(rx_postfc_tvalid_r), .out_tready(rx_postfc_tready_r),  //RX Output
         .busy(rx_flow_ctrl_busy));

   end else begin    //for SOURCE_FLOW_CONTROL == 1

      assign ctrl_tdata_proc  = ctrl_tdata_r;
      assign ctrl_tlast_proc  = ctrl_tlast_r;
      assign ctrl_tvalid_proc = ctrl_tvalid_r;
      assign ctrl_tready_r    = ctrl_tready_proc;

      assign rx_postfc_tdata_r   = rx_prefc_tdata_r;
      assign rx_postfc_tlast_r   = rx_prefc_tlast_r;
      assign rx_postfc_tvalid_r  = rx_prefc_tvalid_r;
      assign rx_prefc_tready_r   = rx_postfc_tready_r;

      assign rx_flow_ctrl_busy   = 1'b0;

   end

endgenerate

   // /////////////////////////////////////////////////////////////////////////////////
   //  TX Chain

   wire [175:0] txsample_tdata;
   wire 	txsample_tvalid, txsample_tready;
   wire [31:0] 	sample_tx;
   wire 	ack_or_error, packet_consumed;
   wire [11:0] 	seqnum;
   wire [63:0] 	error_code;
   wire [31:0] 	sid;
   wire [23:0] tx_fe_i, tx_fe_q;

   wire [31:0] debug_tx_control;

   always @(posedge radio_clk) begin
      tx[31:16] <= (run_tx) ? tx_fe_i[23:8] : tx_idle[31:16];
      tx[15:0]  <= (run_tx) ? tx_fe_q[23:8] : tx_idle[15:0];
   end

   wire [63:0] tx_tdata_i; wire tx_tlast_i, tx_tvalid_i, tx_tready_i;

   new_tx_deframer tx_deframer
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .i_tdata(tx_tdata_i), .i_tlast(tx_tlast_i), .i_tvalid(tx_tvalid_i), .i_tready(tx_tready_i),
      .sample_tdata(txsample_tdata), .sample_tvalid(txsample_tvalid), .sample_tready(txsample_tready),
      .debug());

   new_tx_control #(.BASE(SR_TX_CTRL)) tx_control
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time),
      .ack_or_error(ack_or_error), .packet_consumed(packet_consumed),
      .seqnum(seqnum), .error_code(error_code), .sid(sid),
      .sample_tdata(txsample_tdata), .sample_tvalid(txsample_tvalid), .sample_tready(txsample_tready),
      .sample(sample_tx), .run(run_tx), .strobe(strobe_tx),
      .debug(debug_tx_control));

   tx_responder #(.BASE(SR_TX_CTRL+2)) tx_responder
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .ack_or_error(ack_or_error), .packet_consumed(packet_consumed),
      .seqnum(seqnum), .error_code(error_code), .sid(sid),
      .vita_time(vita_time),
      .o_tdata(txresp_tdata_r), .o_tlast(txresp_tlast_r), .o_tvalid(txresp_tvalid_r), .o_tready(txresp_tready_r));

   wire [31:0]       debug_duc_chain;
   duc_chain #(.BASE(SR_TX_DSP), .DSPNO(0), .WIDTH(24), .NEW_HB_INTERP(NEW_HB_INTERP),.DEVICE(DEVICE)) duc_chain
     (.clk(radio_clk), .rst(radio_rst), .clr(1'b0),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .tx_fe_i(tx_fe_i),.tx_fe_q(tx_fe_q),
      .sample(sample_tx), .run(run_tx), .strobe(strobe_tx),
      .debug(debug_duc_chain) );

`ifdef DELETE_FORMAT_CONVERSION
   assign 	     tx_tdata_i = tx_tdata_r;
   assign 	     tx_tlast_i = tx_tlast_r;
   assign 	     tx_tvalid_i = tx_tvalid_r;
   assign 	     tx_tready_r = tx_tready_i;
`else
    chdr_xxxx_to_16sc_chain #(.BASE(SR_TX_FMT)) convert_xxxx_to_16sc
     (.clk(radio_clk), .reset(radio_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .i_tdata(tx_tdata_r), .i_tlast(tx_tlast_r), .i_tvalid(tx_tvalid_r), .i_tready(tx_tready_r),
      .o_tdata(tx_tdata_i), .o_tlast(tx_tlast_i), .o_tvalid(tx_tvalid_i), .o_tready(tx_tready_i),
      .debug());
`endif // !`ifdef DELETE_FORMAT_CONVERSION

   // /////////////////////////////////////////////////////////////////////////////////
   //  RX Chain

   wire 	full, eob_rx;
   wire 	strobe_rx;
   wire [31:0] 	sample_rx;
   wire [31:0] 	  rx_sid;
   wire [11:0] 	  rx_seqnum;
   wire [63:0] rx_tdata_i; wire rx_tlast_i, rx_tvalid_i, rx_tready_i;
   reg  [1:0]       rx_err_delay_cnt = 2'd0;
   reg              rx_err_gate = 0;

   wire [31:0] debug_rx_framer;
   new_rx_framer #(.BASE(SR_RX_CTRL+4),.SAMPLE_FIFO_SIZE(SAMPLE_FIFO_SIZE)) new_rx_framer
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time),
      .strobe(strobe_rx), .sample(sample_rx), .run(run_rx), .eob(eob_rx), .full(full),
      .sid(rx_sid), .seqnum(rx_seqnum),
      .o_tdata(rx_tdata_i), .o_tlast(rx_tlast_i), .o_tvalid(rx_tvalid_i), .o_tready(rx_tready_i),
      .debug(debug_rx_framer));

   wire [31:0]       debug_rx_control;
   new_rx_control #(.BASE(SR_RX_CTRL)) new_rx_control
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .vita_time(vita_time),
      .strobe(strobe_rx), .run(run_rx), .eob(eob_rx), .full(full),
      .sid(rx_sid), .seqnum(rx_seqnum),
      .err_tdata(rx_err_tdata_r), .err_tlast(rx_err_tlast_r), .err_tvalid(rx_err_tvalid_r), .err_tready(rx_err_gate & rx_err_tready_r),
      .ibs_state(ibs_state),
      .debug(debug_rx_control));

   wire [31:0] 	     debug_ddc_chain;

   // Digital Loopback TX -> RX (Pipeline immediately inside rx_frontend).
   wire [31:0] 	     rx_fe = loopback ? tx : rx;

   ddc_chain #(.BASE(SR_RX_DSP), .DSPNO(0), .WIDTH(24), .NEW_HB_DECIM(NEW_HB_DECIM), .DEVICE(DEVICE)) ddc_chain
     (.clk(radio_clk), .rst(radio_rst), .clr(1'b0),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .rx_fe_i({rx_fe[31:16],8'd0}),.rx_fe_q({rx_fe[15:0],8'd0}),
      .sample(sample_rx), .run(run_rx), .strobe(strobe_rx),
      .debug(debug_ddc_chain) );

`ifdef DELETE_FORMAT_CONVERSION
   assign 	     rx_prefc_tdata_r = rx_tdata_i;
   assign 	     rx_prefc_tlast_r = rx_tlast_i;
   assign 	     rx_prefc_tvalid_r = rx_tvalid_i;
   assign 	     rx_tready_i = rx_prefc_tready_r;
`else
   chdr_16sc_to_xxxx_chain #(.BASE(SR_RX_FMT)) convert_16sc_to_xxxx
     (.clk(radio_clk), .reset(radio_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .i_tdata(rx_tdata_i), .i_tlast(rx_tlast_i), .i_tvalid(rx_tvalid_i), .i_tready(rx_tready_i),
      .o_tdata(rx_prefc_tdata_r), .o_tlast(rx_prefc_tlast_r), .o_tvalid(rx_prefc_tvalid_r), .o_tready(rx_prefc_tready_r),
      .debug());
`endif
   // /////////////////////////////////////////////////////////////////////////////////
   //  RX Channel Muxing

   // The RX framer and converters can add up to 3 bubble cycles on the data path.
   // Delay any error packets so errors are back in band with the data packets.
   always @(posedge radio_clk) begin
      if (radio_rst) begin
         rx_err_delay_cnt <= 2'd0;
         rx_err_gate <= 0;
      end else begin
         if (rx_err_gate) begin
            if (rx_err_tvalid_r & rx_err_tready_r & rx_err_tlast_r) begin
               rx_err_delay_cnt <= 2'd0;
               rx_err_gate <= 0;
            end
         end else begin
            if (rx_postfc_tvalid_r) begin
               rx_err_delay_cnt <= 2'd0;
            end else if (rx_err_delay_cnt == 2'd3) begin
               rx_err_gate <= 1;
            end else if (rx_err_tvalid_r) begin
               rx_err_delay_cnt <= rx_err_delay_cnt + 2'd1;
            end
         end
      end
   end

   axi_mux4 #(.PRIO(1), .WIDTH(64), .BUFFER(1)) rx_mux
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .i0_tdata(rx_postfc_tdata_r), .i0_tlast(rx_postfc_tlast_r), .i0_tvalid(rx_postfc_tvalid_r), .i0_tready(rx_postfc_tready_r),
      .i1_tdata(rx_err_tdata_r), .i1_tlast(rx_err_gate & rx_err_tlast_r), .i1_tvalid(rx_err_gate & rx_err_tvalid_r), .i1_tready(rx_err_tready_r),
      .i2_tdata(64'h0), .i2_tlast(1'b0), .i2_tvalid(1'b0), .i2_tready(),
      .i3_tdata(64'h0), .i3_tlast(1'b0), .i3_tvalid(1'b0), .i3_tready(),
      .o_tdata(rx_tdata_r), .o_tlast(rx_tlast_r), .o_tvalid(rx_tvalid_r), .o_tready(rx_tready_r));

   // /////////////////////////////////////////////////////////////////////////////////
   //  Response Channel Muxing

   axi_mux4 #(.PRIO(0), .WIDTH(64)) response_mux
     (.clk(radio_clk), .reset(radio_rst), .clear(1'b0),
      .i0_tdata(txresp_tdata_r), .i0_tlast(txresp_tlast_r), .i0_tvalid(txresp_tvalid_r), .i0_tready(txresp_tready_r),
      .i1_tdata(resp_tdata_r), .i1_tlast(resp_tlast_r), .i1_tvalid(resp_tvalid_r), .i1_tready(resp_tready_r),
      .i2_tdata(64'h0), .i2_tlast(1'b0), .i2_tvalid(1'b0), .i2_tready(),
      .i3_tdata(64'h0), .i3_tlast(1'b0), .i3_tvalid(1'b0), .i3_tready(),
      .o_tdata(rmux_tdata_r), .o_tlast(rmux_tlast_r), .o_tvalid(rmux_tvalid_r), .o_tready(rmux_tready_r));




   /*******************************************************************
    * Debug only logic below here.
    ******************************************************************/
 assign debug = 0;

endmodule // radio_legacy
