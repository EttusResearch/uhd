//
// Copyright 2021 Ettus Research, A National Instruments brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_100g
//
// Description: Wrapper for the Xilinx 100G mac


module eth_100g #(
    logic        PAUSE_EN = 1,
    logic [15:0] PAUSE_QUANTA  = 16'hFFFF,
    logic [15:0] PAUSE_REFRESH = 16'hFFFF
  )(

  // Resets
  input  logic        areset,
  // Clock for misc stuff
  input  logic        clk100,
  // Low jitter refclk
  input  logic        refclk_p,
  input  logic        refclk_n,
  // RX Clk for output
  output logic        rx_rec_clk_out,
  // MGT high-speed IO
  output logic[3:0]   tx_p,
  output logic[3:0]   tx_n,
  input  logic[3:0]   rx_p,
  input  logic[3:0]   rx_n,

  // Data port
  output logic        mgt_clk,
  output logic        mgt_rst,
  input  logic        mgt_pause_req,
  // Interface clocks for mgt_tx and mgt_rx are NOT used (logic uses mgt_clk)
  AxiStreamIf.slave   mgt_tx,
  AxiStreamIf.master  mgt_rx,
  // Axi port
  AxiLiteIf.slave     mgt_axil,
  // Misc
  output logic [31:0] phy_status,
  input  logic [31:0] mac_ctrl,
  output logic [31:0] mac_status,
  output logic        phy_reset,
  output logic        link_up
);

  logic  tx_ovfout;
  logic  tx_unfout;
  logic  stat_rx_aligned;
  logic  stat_auto_config_done;
  logic  stat_auto_config_done_bclk;
  logic  usr_tx_reset;
  logic  usr_rx_reset;

  // Hierarchical reference (Xilinx says it will synthesize)
  // eth_100g_bd_i/cmac_usplus_0/gt_rxrecclkout}
  assign rx_rec_clk_out = eth_100g_bd_i.cmac_usplus_0.gt_rxrecclkout[0];
  //status registers
  always_comb begin
    phy_status = 0;
    phy_status[0] = usr_tx_reset;
    phy_status[1] = usr_rx_reset;

  end

  logic [8:0] pause_mask;   // from mac ctl register bits 24:16

  always_ff @(posedge mgt_clk) begin : mac_status_reg
    if (mgt_rst) begin
      mac_status <= 0;
    end else begin
      mac_status[0] <= mac_status[0] || tx_ovfout;
      mac_status[1] <= mac_status[1] || tx_unfout;
      mac_status[2] <= stat_rx_aligned;
      mac_status[3] <= mac_status[3] || (!mgt_rx.tready && mgt_rx.tvalid);
      mac_status[4] <= stat_auto_config_done;
      mac_status[24:16] <= pause_mask;
    end
  end

  // synthesis translate_off
  //extra simulation checks
  localparam USE_MAC_CHECKS = 1;
  if (USE_MAC_CHECKS) begin
    always_ff @(posedge mgt_rx.clk) begin : check_no_holdoff
      if (!mgt_rx.rst) begin
        if (!mgt_rx.tready && mgt_rx.tvalid) begin
          $fatal(1,"MAC RX can't hold off the MAC");
        end
        assert(tx_ovfout==0) else
          $fatal(1,"MAC TX had an overflow!");
        assert(tx_unfout==0) else
          $fatal(1,"MAC TX had an underflow!");
      end
    end
  end

  initial begin
    assert (mgt_tx.DATA_WIDTH == 512) else
     $fatal(1, "mgt_rx.DATA_WIDTH must be 512");
    // $clog2(512/8)+1
    assert (mgt_rx.USER_WIDTH == 7) else
     $fatal(1, "mgt_rx.USER_WIDTH must be 7");
    assert (mgt_tx.TDATA == 1) else
     $fatal(1, "mgt_tx.TDATA must be enabled");
    assert (mgt_tx.TUSER == 1) else
     $fatal(1, "mgt_tx.TUSER must be enabled");
    assert (mgt_tx.TKEEP == 1) else
     $fatal(1, "mgt_tx.TKEEP must be enabled");
    assert (mgt_tx.TLAST == 1) else
     $fatal(1, "mgt_tx.TLAST must be enabled");
    assert (mgt_rx.DATA_WIDTH == 512) else
     $fatal(1, "mgt_rx.DATA_WIDTH must be 512");
    // $clog2(512/8)+1
    assert (mgt_rx.USER_WIDTH == 7) else
     $fatal(1, "mgt_rx.DATA_WIDTH must be 7");
    assert (mgt_rx.TDATA == 1) else
     $fatal(1, "mgt_rx.TDATA must be enabled");
    assert (mgt_rx.TUSER == 1) else
     $fatal(1, "mgt_rx.TUSER must be enabled");
    assert (mgt_rx.TKEEP == 0) else
     $fatal(1, "mgt_rx.TKEEP must not be enabled");
    assert (mgt_rx.TLAST == 1) else
     $fatal(1, "mgt_rx.TLAST must be enabled");
  end
  // synthesis translate_on

  AxiStreamIf #(.DATA_WIDTH(512),.TUSER(0),.TKEEP(0))
    eth100g_tx(mgt_clk,mgt_rst);
  AxiStreamIf #(.DATA_WIDTH(512),.USER_WIDTH(7),.TKEEP(0))
    eth100g_rx(mgt_clk,mgt_rst);

  logic mgt_tx_idle;
  logic mgt_tx_pause;

  always_comb begin
    eth100g_tx.tdata  = mgt_tx.tdata;
    eth100g_tx.tuser  = 0;
    eth100g_tx.tkeep  = mgt_tx.tkeep;
    eth100g_tx.tvalid = mgt_tx.tvalid && !mgt_tx_pause;
    eth100g_tx.tlast  = mgt_tx.tlast;
    mgt_tx.tready = eth100g_tx.tready && !mgt_tx_pause;
  end

  always_comb begin
    mgt_rx.tdata    = eth100g_rx.tdata;
    mgt_rx.tuser    = eth100g_rx.tuser;
    mgt_rx.tuser[mgt_rx.USER_WIDTH-1] = // assign error bit [MSB]
      // CRC failure
      eth100g_rx.tuser[mgt_rx.USER_WIDTH-1] ||
      // Missed a DATA word.
      mgt_rx.tvalid && !mgt_rx.tready;
    mgt_rx.tkeep    = eth100g_rx.tkeep;
    mgt_rx.tvalid   = eth100g_rx.tvalid;
    mgt_rx.tlast    = eth100g_rx.tlast;
    // The MAC ignores hold off.  Data must be consumed every clock it is valid.
    // eth100g_rx.tready = mgt_rx.tready;
  end

  // This is a heavily replicated signal, add some pipeline
  // to it to make it easier to spread out
  logic mgt_rst_0;


  // Flow control signals
  // 0-7 map to PCP codes 0-7.  8 is a global pause request
  logic [8:0] stat_rx_pause_req ;
  logic [8:0] ctl_tx_pause_req ; // drive for at least 16 clocks
  logic       ctl_tx_resend_pause; // resend the pause request (tying this high forces a spam of resend requests)

  // QuantaPeriod is 512 bit times or 5.12 ns
  // resend pause requests so (quanta*QuantaPeriod)/(refresh*QuantaPeriod) is the percentage of BW that gets through.
  // pause_mask is part of the mac_ctrl register
  always_comb begin
    ctl_tx_resend_pause = 0;
    ctl_tx_pause_req = '0;
    if (mgt_pause_req) begin
      ctl_tx_pause_req = pause_mask;
    end
  end
  logic mgt_tx_pause_req;
  assign mgt_tx_pause_req = (pause_mask & stat_rx_pause_req) != 0;
  always_ff @(posedge mgt_clk,posedge areset) begin : reset_timing_dff
    if (areset) begin
      mgt_rst_0    <= 1'b1;
      mgt_rst      <= 1'b1;
      mgt_tx_pause <= 1'b0;
      mgt_tx_idle  <= 1'b1;
    end else begin
      mgt_rst_0 <= !link_up;
      mgt_rst   <= mgt_rst_0;
      //idle until a valid sets
      if (mgt_tx_idle) begin
        if (!eth100g_tx.tvalid) begin
          mgt_tx_pause <= mgt_tx_pause_req;
        end else begin
          // one clock packet
          if (eth100g_tx.tvalid && eth100g_tx.tlast && eth100g_tx.tready) begin
            mgt_tx_idle <= 1;
            mgt_tx_pause <= mgt_tx_pause_req;
          end else begin
            mgt_tx_idle <= 0;
          end
        end
      //set idle if end of packet is accepted
      end else if (eth100g_tx.tvalid && eth100g_tx.tlast && eth100g_tx.tready) begin
        mgt_tx_idle  <= 1;
        mgt_tx_pause <= mgt_tx_pause_req;
      end
    end
  end

  always_comb phy_reset = usr_tx_reset || usr_rx_reset;
  always_comb link_up =  stat_rx_aligned && !phy_reset;

  // resets stat counts and moves the total to the readable version.
  localparam PM_COUNT = 40000;
  logic pm_tick = 0;
  logic [15:0] pm_tick_count;
  always_ff @(posedge mgt_axil.clk) begin : pm_tick_counter
    if (mgt_axil.rst) begin
      pm_tick_count = 0;
      pm_tick = 0;
    end else begin
      if (pm_tick_count == PM_COUNT-1) begin
        pm_tick_count = 0;
        pm_tick = 1;
      end else begin
        pm_tick_count = pm_tick_count+1;
        pm_tick = 0;
      end
    end
  end

  `include "../../../../lib/axi4lite_sv/axi_lite.vh"
  AxiLiteIf_v #(.DATA_WIDTH(mgt_axil.DATA_WIDTH),.ADDR_WIDTH(32))
    mgt_axil_v(.clk(mgt_axil.clk),.rst(mgt_axil.rst));

  localparam AUTO_CONNECT=1;
  // When enabled the port will automatically attempt to connect to an Ethernet partner
  // without requiring any action from SW.  If it is not defined, SW will have to perform
  // a similar set of writes. Xilinx publishes a driver for the MAC, that we could associate.
  // The sequence of writes was taken from the CMAC example, without any deep knowledge
  // of what the standard Ethernet connection protocol is.

  // Inject writes to perform connection in between other SW writes to read the mac.
  if (AUTO_CONNECT) begin : yes_auto_connect
    // defined in https://www.xilinx.com/support/documentation/ip_documentation/cmac_usplus/v2_4/pg203-cmac-usplus.pdf
    // pg 187
    localparam CONFIGURATION_TX_REG1 = 32'h000C;
    localparam ctl_tx_ctl_enable       = 0;
    localparam ctl_tx_ctl_tx_send_lfi  = 3;
    localparam ctl_tx_ctl_tx_send_rfi  = 4;
    localparam ctl_tx_ctl_tx_send_idle = 5;
    localparam ctl_tx_ctl_test_pattern = 16;

    localparam CONFIGURATION_RX_REG1 = 32'h0014;
    localparam ctl_rx_ctl_enable           = 0;
    localparam ctl_rx_ctl_rx_force_resync  = 7;
    localparam ctl_rx_ctl_test_pattern     = 8;

    localparam RSFEC_CONFIG_INDICATION_CORRECTION = 32'h1000;
    localparam rs_fec_in_ctl_rx_rsfec_enable_correction       = 0;
    localparam rs_fec_in_ctl_rx_rsfec_enable_indication       = 1;
    localparam rs_fec_in_ctl_rsfec_ieee_error_indication_mode = 2;

    localparam RSFEC_CONFIG_ENABLE = 32'h107C;
    localparam rs_fec_in_ctl_rx_rsfec_enable = 0;
    localparam rs_fec_in_ctl_tx_rsfec_enable = 1;

    // Extra configuration for Pause Frames (CMAC guide Pg 210)
    //0x0084 : 32'h00003DFF [CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG1]
    //0x0088 : 32'h0001C631 [CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG2]
    //0x0048 : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG1]
    //0x004C : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG2]
    //0x0050 : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG3]
    //0x0054 : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG4]
    //0x0058 : 32'h0000FFFF [CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG5]
    //0x0034 : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG1]
    //0x0038 : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG2]
    //0x003C : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG3]
    //0x0040 : 32'hFFFFFFFF [CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG4]
    //0x0044 : 32'h0000FFFF [CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG5]
    //0x0030 : 32'h000001FF [CONFIGURATION_TX_FLOW_CONTROL_CONTROL_REG1]

    localparam CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG1  = 32'h0084;
    //3DFF - 0011 1101 1111 1111
    localparam ctl_rx_pause_en   = 0; // 9 bits
    localparam ctl_rx_enable_gcp = 10;
    localparam ctl_rx_enable_pcp = 11;
    localparam ctl_rx_enable_gpp = 12;
    localparam ctl_rx_enable_ppp = 13;
    localparam ctl_rx_pause_ack  = 23; // 8 bits
    localparam CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG2  = 32'h0088;
    //1C631 - 0001 1100 0110 0011 0001
    localparam ctl_rx_check_mcast_gcp     = 0; //1
    localparam ctl_rx_check_ucast_gcp     = 1;
    localparam ctl_rx_check_sa_gcp        = 2;
    localparam ctl_rx_check_etype_gcp     = 3;
    localparam ctl_rx_check_opcode_gcp    = 4; //1
    localparam ctl_rx_check_mcast_pcp     = 5; //1
    localparam ctl_rx_check_ucast_pcp     = 6;
    localparam ctl_rx_check_sa_pcp        = 7;
    localparam ctl_rx_check_etype_pcp     = 8;
    localparam ctl_rx_check_opcode_pcp    = 9; //1
    localparam ctl_rx_check_mcast_gpp    = 10; //1
    localparam ctl_rx_check_ucast_gpp    = 11;
    localparam ctl_rx_check_sa_gpp       = 12;
    localparam ctl_rx_check_etype_gpp    = 13;
    localparam ctl_rx_check_opcode_gpp   = 14; //1
    localparam ctl_rx_check_opcode_ppp   = 15; //1
    localparam ctl_rx_check_mcast_ppp    = 16; //1
    localparam ctl_rx_check_ucast_ppp    = 17;
    localparam ctl_rx_check_sa_ppp       = 18;
    localparam ctl_rx_check_etype_ppp    = 19;

    localparam CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG1   = 32'h0048;
    localparam ctl_tx_pause_quanta0 = 0;
    localparam ctl_tx_pause_quanta1 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG2   = 32'h004C;
    localparam ctl_tx_pause_quanta2 = 0;
    localparam ctl_tx_pause_quanta3 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG3   = 32'h0050;
    localparam ctl_tx_pause_quanta4 = 0;
    localparam ctl_tx_pause_quanta5 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG4   = 32'h0054;
    localparam ctl_tx_pause_quanta6 = 0;
    localparam ctl_tx_pause_quanta7 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG5   = 32'h0058;
    localparam ctl_tx_pause_quanta8 = 0;
    localparam CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG1  = 32'h0034;
    localparam ctl_tx_pause_refresh_timer0 = 0;
    localparam ctl_tx_pause_refresh_timer1 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG2  = 32'h0038;
    localparam ctl_tx_pause_refresh_timer2 = 0;
    localparam ctl_tx_pause_refresh_timer3 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG3  = 32'h003C;
    localparam ctl_tx_pause_refresh_timer4 = 0;
    localparam ctl_tx_pause_refresh_timer5 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG4  = 32'h0040;
    localparam ctl_tx_pause_refresh_timer6 = 0;
    localparam ctl_tx_pause_refresh_timer7 = 16;
    localparam CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG5  = 32'h0044;
    localparam ctl_tx_pause_refresh_timer8 = 0;
    localparam CONFIGURATION_TX_FLOW_CONTROL_CONTROL_REG1  = 32'h0030;
    // 1FF
    localparam ctl_tx_pause_enable  = 0; // 9 bits


    AxiLiteIf #(.DATA_WIDTH(mgt_axil.DATA_WIDTH),.ADDR_WIDTH(32))
      auto_axil(.clk(mgt_axil.clk),.rst(mgt_axil.rst));

    typedef enum logic [4:0] {
      ST_RESET                                         = 5'd0,
      ST_WR_CONFIGURATION_TX_REG1_IDLE                 = 5'd1,
      ST_WR_RSFEC_CONFIG_INDICATION_CORRECTION         = 5'd2,
      ST_WR_RSFEC_CONFIG_ENABLE                        = 5'd3,
      ST_WR_CONFIGURATION_RX_REG1                      = 5'd4,
      ST_WAIT                                          = 5'd5,
      ST_WR_CONFIGURATION_TX_REG1_TX_ENABLE            = 5'd6,
      ST_READY                                         = 5'd7,
      // EXTRA PAUSE WRITES
      ST_WR_CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG1 = 5'd8,
      ST_WR_CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG2 = 5'd9,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG1  = 5'd10,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG2  = 5'd11,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG3  = 5'd12,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG4  = 5'd13,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG5  = 5'd14,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG1 = 5'd15,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG2 = 5'd16,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG3 = 5'd17,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG4 = 5'd18,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG5 = 5'd19,
      ST_WR_CONFIGURATION_TX_FLOW_CONTROL_CONTROL_REG1 = 5'd20
    } auto_connect_state_t;

    auto_connect_state_t auto_connect_state = ST_RESET;
    logic auto_rst0, auto_rst1, auto_rst2, auto_rst3;
    logic mgt_axil_in_progress;
    logic w_req, aw_req;
    logic phy_reset_bclk,stat_rx_aligned_bclk;
    logic auto_enable;

    synchronizer #( .STAGES(2), .WIDTH(1), .INITIAL_VAL(0) ) phy_reset_sync_i (
      .clk(mgt_axil.clk), .rst(1'b0), .in(phy_reset), .out(phy_reset_bclk)
    );

    synchronizer #( .STAGES(2), .WIDTH(1), .INITIAL_VAL(0) ) rx_aligned_sync_i (
      .clk(mgt_axil.clk), .rst(1'b0), .in(stat_rx_aligned), .out(stat_rx_aligned_bclk)
    );

   synchronizer #( .STAGES(2), .WIDTH(1), .INITIAL_VAL(0) ) auto_config_done_sync_i (
      .clk(mgt_clk), .rst(1'b0), .in(stat_auto_config_done_bclk), .out(stat_auto_config_done)
    );


    synchronizer #( .STAGES(2), .WIDTH(1), .INITIAL_VAL(0) ) auto_enable_sync_i (
      .clk(mgt_axil.clk), .rst(1'b0), .in(mac_ctrl[0]), .out(auto_enable)
    );

    synchronizer #( .STAGES(2), .WIDTH(9), .INITIAL_VAL(9'h100) ) pause_mask_sync_i (
      .clk(mgt_clk), .rst(1'b0), .in(mac_ctrl[24:16]), .out(pause_mask)
    );


    always_ff @(posedge mgt_axil.clk) begin : auto_enable_logic
      if (mgt_axil.rst) begin
        auto_rst0 <= 1'b1;
        auto_rst1 <= 1'b1;
        auto_rst2 <= 1'b1;
        auto_rst3 <= 1'b1;
        auto_connect_state <= ST_RESET;
        stat_auto_config_done_bclk <= 1'b0;
        mgt_axil_in_progress <= 1'b0;
        w_req  <= 1'b0;
        aw_req <= 1'b0;

        // default is to drive mgt_axi_through
        /* write address channel */
        auto_axil.awaddr    <= 'bX;
        auto_axil.awvalid   <= 1'b0;
        mgt_axil.awready    <= 1'b0;
        /* write data channel */
        auto_axil.wdata     <= 'bX;
        auto_axil.wstrb     <= 'b0;
        auto_axil.wvalid    <= 1'b0;
        mgt_axil.wready     <= 1'b0;
        /* write resp channel */
        mgt_axil.bresp[1:0] <= 'b0;
        mgt_axil.bvalid     <= 1'b0;
        auto_axil.bready    <= 1'b0;
        /* read address channel */
        auto_axil.araddr    <= 'b0;
        auto_axil.arvalid   <= 1'b0;
        mgt_axil.arready    <= 1'b0;
        /* read resp channel */
        mgt_axil.rdata      <= 'bX;
        mgt_axil.rresp[1:0] <= 'b0;
        mgt_axil.rvalid     <= 1'b0;
        auto_axil.rready    <= 1'b0;

      end else begin
        // 4 clocks to mimic Xilinx Example behavior
        auto_rst0 <= phy_reset_bclk;
        auto_rst1 <= auto_rst0;
        auto_rst2 <= auto_rst1;
        auto_rst3 <= auto_rst2;
        // assumes one access in flight at time (valid for standard Xilinx AXIL driver)
        // set if anyone starts driving a W Address / W Data / R Address channel
        if (auto_axil.awvalid || auto_axil.wvalid || auto_axil.arvalid) begin
          mgt_axil_in_progress <= 1'b1;
        // clear on an acknowledged response
        end else if ((auto_axil.bvalid && auto_axil.bready) ||
                     (auto_axil.rvalid && auto_axil.rready)) begin
          mgt_axil_in_progress <= 1'b0;
        end

        // default is to drive mgt_axi_through
        /* write address channel */
        auto_axil.awaddr  <= mgt_axil.awaddr;
        auto_axil.awvalid <= mgt_axil.awvalid;
        mgt_axil.awready  <= auto_axil.awready;
        /* write data channel */
        auto_axil.wdata   <= mgt_axil.wdata;
        auto_axil.wstrb   <= mgt_axil.wstrb;
        auto_axil.wvalid  <= mgt_axil.wvalid;
        mgt_axil.wready   <= auto_axil.wready;
        /* write resp channel */
        mgt_axil.bresp    <= auto_axil.bresp;
        mgt_axil.bvalid   <= auto_axil.bvalid;
        auto_axil.bready  <= mgt_axil.bready;
        /* read address channel */
        auto_axil.araddr  <= mgt_axil.araddr;
        auto_axil.arvalid <= mgt_axil.arvalid;
        mgt_axil.arready  <= auto_axil.arready;
        /* read resp channel */
        mgt_axil.rdata    <= auto_axil.rdata;
        mgt_axil.rresp    <= auto_axil.rresp;
        mgt_axil.rvalid   <= auto_axil.rvalid;
        auto_axil.rready  <= mgt_axil.rready;

        if (auto_rst3) begin
          auto_connect_state = ST_RESET;
          stat_auto_config_done_bclk <= 1'b0;
        end else begin
          case (auto_connect_state)

            ST_RESET: begin
              stat_auto_config_done_bclk <= 1'b0;
              if (!mgt_axil_in_progress && auto_enable) begin
                 mgt_axil.awready <= 0;
                 mgt_axil.wready  <= 0;
                 mgt_axil.arready <= 0;
                 auto_connect_state <= ST_WR_CONFIGURATION_TX_REG1_IDLE;
                 w_req  <= 1'b1;
                 aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_REG1_IDLE: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // start transmitting alignment pattern
              auto_axil.wdata   <= 0;
              auto_axil.wdata[ctl_tx_ctl_enable] <= 0;
              auto_axil.wdata[ctl_tx_ctl_tx_send_idle] <= 0;
              auto_axil.wdata[ctl_tx_ctl_tx_send_lfi] <= 0;
              auto_axil.wdata[ctl_tx_ctl_tx_send_rfi] <= 1;
              auto_axil.wdata[ctl_tx_ctl_test_pattern] <= 0;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_RSFEC_CONFIG_INDICATION_CORRECTION;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_RSFEC_CONFIG_INDICATION_CORRECTION: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // configure fec
              auto_axil.wdata <= 0;
              auto_axil.wdata[rs_fec_in_ctl_rx_rsfec_enable_correction] <= 1;
              auto_axil.wdata[rs_fec_in_ctl_rx_rsfec_enable_indication] <= 1;
              auto_axil.wdata[rs_fec_in_ctl_rsfec_ieee_error_indication_mode] <= 1;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= RSFEC_CONFIG_INDICATION_CORRECTION;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_RSFEC_CONFIG_ENABLE;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_RSFEC_CONFIG_ENABLE: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // enable fec
              auto_axil.wdata <= 0;
              auto_axil.wdata[rs_fec_in_ctl_rx_rsfec_enable] <= 1;
              auto_axil.wdata[rs_fec_in_ctl_tx_rsfec_enable] <= 1;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= RSFEC_CONFIG_ENABLE;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_RX_REG1;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_RX_REG1: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_rx_ctl_enable] <= 1;
              auto_axil.wdata[ctl_rx_ctl_rx_force_resync] <= 0;
              auto_axil.wdata[ctl_rx_ctl_test_pattern] <= 0;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_RX_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WAIT;
              end
            end

            ST_WAIT: begin
              mgt_axil.awready  <= 0;
              mgt_axil.wready   <= 0;
              mgt_axil.arready  <= 0;
              auto_axil.arvalid <= 0;
              // don't drive any writes, but hold off bus
              auto_axil.wdata   <= 0;
              auto_axil.wstrb   <= 0;
              auto_axil.wvalid  <= 0;
              auto_axil.awaddr  <= 0;
              auto_axil.awvalid <= 0;
              auto_axil.bready  <= 0;
              if (stat_rx_aligned_bclk) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_REG1_TX_ENABLE;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_REG1_TX_ENABLE: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // stop transmitting alignment pattern
              // and start transmitting data
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_ctl_enable] <= 1;
              auto_axil.wdata[ctl_tx_ctl_tx_send_idle] <= 0;
              auto_axil.wdata[ctl_tx_ctl_tx_send_lfi] <= 0;
              auto_axil.wdata[ctl_tx_ctl_tx_send_rfi] <= 0;
              auto_axil.wdata[ctl_tx_ctl_test_pattern] <= 0;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                if (PAUSE_EN) begin
                  auto_connect_state <= ST_WR_CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG1;
                  w_req  <= 1'b1;
                  aw_req <= 1'b1;
                end else begin
                  auto_connect_state <= ST_READY;
                end
              end
            end

            ST_WR_CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG1: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              //3DFF - 0011 1101 1111 1111
              auto_axil.wdata[ctl_rx_pause_en+:9] <= '1;
              auto_axil.wdata[ctl_rx_enable_gcp]  <= 1;
              auto_axil.wdata[ctl_rx_enable_pcp]  <= 1;
              auto_axil.wdata[ctl_rx_enable_gpp]  <= 1;
              auto_axil.wdata[ctl_rx_enable_ppp]  <= 1;
              //ctl_rx_pause_ack  = 23; // 8 bits  NOT SET
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG2;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG2: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              //1C631 - 0001 1100 0110 0011 0001
              auto_axil.wdata[ctl_rx_check_mcast_gcp ] <= 1; //1
              auto_axil.wdata[ctl_rx_check_ucast_gcp ] <= 0;
              auto_axil.wdata[ctl_rx_check_sa_gcp    ] <= 0;
              auto_axil.wdata[ctl_rx_check_etype_gcp ] <= 0;
              auto_axil.wdata[ctl_rx_check_opcode_gcp] <= 1; //1
              auto_axil.wdata[ctl_rx_check_mcast_pcp ] <= 1; //1
              auto_axil.wdata[ctl_rx_check_ucast_pcp ] <= 0;
              auto_axil.wdata[ctl_rx_check_sa_pcp    ] <= 0;
              auto_axil.wdata[ctl_rx_check_etype_pcp ] <= 0;
              auto_axil.wdata[ctl_rx_check_opcode_pcp] <= 1; //1
              auto_axil.wdata[ctl_rx_check_mcast_gpp ] <= 1; //1
              auto_axil.wdata[ctl_rx_check_ucast_gpp ] <= 0;
              auto_axil.wdata[ctl_rx_check_sa_gpp    ] <= 0;
              auto_axil.wdata[ctl_rx_check_etype_gpp ] <= 0;
              auto_axil.wdata[ctl_rx_check_opcode_gpp] <= 1; //1
              auto_axil.wdata[ctl_rx_check_opcode_ppp] <= 1; //1
              auto_axil.wdata[ctl_rx_check_mcast_ppp ] <= 1; //1
              auto_axil.wdata[ctl_rx_check_ucast_ppp ] <= 0;
              auto_axil.wdata[ctl_rx_check_sa_ppp    ] <= 0;
              auto_axil.wdata[ctl_rx_check_etype_ppp ] <= 0;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_RX_FLOW_CONTROL_CONTROL_REG2;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG1;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG1: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_quanta0+:16] <= PAUSE_QUANTA;
              auto_axil.wdata[ctl_tx_pause_quanta1+:16] <= PAUSE_QUANTA;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG2;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG2: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_quanta2+:16] <= PAUSE_QUANTA;
              auto_axil.wdata[ctl_tx_pause_quanta3+:16] <= PAUSE_QUANTA;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG2;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG3;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG3: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_quanta4+:16] <= PAUSE_QUANTA;
              auto_axil.wdata[ctl_tx_pause_quanta5+:16] <= PAUSE_QUANTA;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG3;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG4;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG4: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_quanta6+:16] <= PAUSE_QUANTA;
              auto_axil.wdata[ctl_tx_pause_quanta7+:16] <= PAUSE_QUANTA;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG4;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG5;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG5: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_quanta8+:16] <= PAUSE_QUANTA;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_QUANTA_REG5;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG1;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG1: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_refresh_timer0+:16] <= PAUSE_REFRESH;
              auto_axil.wdata[ctl_tx_pause_refresh_timer1+:16] <= PAUSE_REFRESH;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG2;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG2: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_refresh_timer2+:16] <= PAUSE_REFRESH;
              auto_axil.wdata[ctl_tx_pause_refresh_timer3+:16] <= PAUSE_REFRESH;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG2;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG3;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG3: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_refresh_timer4+:16] <= PAUSE_REFRESH;
              auto_axil.wdata[ctl_tx_pause_refresh_timer5+:16] <= PAUSE_REFRESH;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG3;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG4;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG4: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_refresh_timer6+:16] <= PAUSE_REFRESH;
              auto_axil.wdata[ctl_tx_pause_refresh_timer7+:16] <= PAUSE_REFRESH;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG4;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG5;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG5: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              auto_axil.wdata[ctl_tx_pause_refresh_timer8+:16] <= PAUSE_REFRESH;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_REFRESH_REG5;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_WR_CONFIGURATION_TX_FLOW_CONTROL_CONTROL_REG1;
                w_req  <= 1'b1;
                aw_req <= 1'b1;
              end
            end

            ST_WR_CONFIGURATION_TX_FLOW_CONTROL_CONTROL_REG1: begin
              mgt_axil.awready <= 0;
              mgt_axil.wready  <= 0;
              mgt_axil.arready <= 0;
              auto_axil.arvalid <= 0;
              // turn on RX interface
              auto_axil.wdata <= 0;
              // 1FF
              auto_axil.wdata[ctl_tx_pause_enable+:9] <= '1;
              auto_axil.wstrb   <= '1;
              auto_axil.wvalid  <= w_req;
              auto_axil.awaddr  <= CONFIGURATION_TX_FLOW_CONTROL_CONTROL_REG1;
              auto_axil.awvalid <= aw_req;
              auto_axil.bready  <= 1'b1;
              if (auto_axil.wready)  begin
                auto_axil.wvalid  <= 1'b0;
                w_req             <= 1'b0;
              end
              if (auto_axil.awready) begin
                auto_axil.awvalid <= 1'b0;
                aw_req            <= 1'b0;
              end
              if (auto_axil.bvalid) begin
                auto_connect_state <= ST_READY;
              end
            end

            ST_READY: begin
              stat_auto_config_done_bclk <= 1'b1;
              if (!stat_rx_aligned_bclk) begin
                auto_connect_state <= ST_RESET;
              end
            end

          endcase
        end
      end
    end

    always_comb begin
      `AXI4LITE_ASSIGN(mgt_axil_v,auto_axil)

      // window address to 0x0000-x1FFF
      mgt_axil_v.araddr        = 0;
      mgt_axil_v.araddr[12:0]  = auto_axil.araddr[12:0];
      // window address to 0x0000-x1FFF
      mgt_axil_v.awaddr       = 0;
      mgt_axil_v.awaddr[12:0] = auto_axil.awaddr[12:0];
    end

  end else begin : no_auto_connect

    always_comb begin
      `AXI4LITE_ASSIGN(mgt_axil_v,mgt_axil)

      // window address to 0x0000-x1FFF
      mgt_axil_v.araddr        = 0;
      mgt_axil_v.araddr[12:0]  = mgt_axil.araddr[12:0];
      // window address to 0x0000-x1FFF
      mgt_axil_v.awaddr       = 0;
      mgt_axil_v.awaddr[12:0] = mgt_axil.awaddr[12:0];

      stat_auto_config_done_bclk = 1'b1;
    end
  end

  import PkgEth100gLbus::*;


  lbus_t lbus_rx [3:0];
  lbus_t lbus_tx [3:0];
  logic lbus_tx_rdyout;

  eth_100g_lbus2axi  #(.NUM_SEG(4)) lbus2axi (
    .axis(eth100g_rx),
    .lbus_in(lbus_rx)
  );

  eth_100g_axi2lbus  #(.NUM_SEG(4)) axi2lbus (
    .axis(eth100g_tx),
    .lbus_rdy(lbus_tx_rdyout),
    .lbus_out(lbus_tx)
  );

  eth_100g_bd eth_100g_bd_i (
    .refclk_clk_n(refclk_n),
    .refclk_clk_p(refclk_p),
    .gt_serial_port_0_grx_n(rx_n),
    .gt_serial_port_0_grx_p(rx_p),
    .gt_serial_port_0_gtx_n(tx_n),
    .gt_serial_port_0_gtx_p(tx_p),
    .init_clk(clk100),
    .sys_reset(areset),
    .usr_rx_reset(usr_rx_reset),
    .usr_tx_reset(usr_tx_reset),
    .gt_txusrclk2(mgt_clk),
    .rx_clk(mgt_clk), //feedback in
    .tx_ovfout(tx_ovfout),
    .tx_unfout(tx_unfout),
    .stat_rx_aligned(stat_rx_aligned),
    .drp_clk(clk100),
    .core_drp_daddr(10'b0),
    .core_drp_den(1'b0),
    .core_drp_di(16'b0),
    .core_drp_do(),
    .core_drp_drdy(),
    .core_drp_dwe(1'b0),
    `AXI4LITE_PORT_ASSIGN(s_axi,mgt_axil_v)
    .pm_tick(pm_tick),
    .ctl_tx_pause_req(ctl_tx_pause_req),
    .ctl_tx_resend_pause(ctl_tx_resend_pause),
    .stat_rx_pause_req(stat_rx_pause_req),
    .eth100g_rx_lbus_seg0_data(lbus_rx[0].data),
    .eth100g_rx_lbus_seg0_ena(lbus_rx[0].ena),
    .eth100g_rx_lbus_seg0_eop(lbus_rx[0].eop),
    .eth100g_rx_lbus_seg0_err(lbus_rx[0].err),
    .eth100g_rx_lbus_seg0_mty(lbus_rx[0].mty),
    .eth100g_rx_lbus_seg0_sop(lbus_rx[0].sop),
    .eth100g_rx_lbus_seg1_data(lbus_rx[1].data),
    .eth100g_rx_lbus_seg1_ena(lbus_rx[1].ena),
    .eth100g_rx_lbus_seg1_eop(lbus_rx[1].eop),
    .eth100g_rx_lbus_seg1_err(lbus_rx[1].err),
    .eth100g_rx_lbus_seg1_mty(lbus_rx[1].mty),
    .eth100g_rx_lbus_seg1_sop(lbus_rx[1].sop),
    .eth100g_rx_lbus_seg2_data(lbus_rx[2].data),
    .eth100g_rx_lbus_seg2_ena(lbus_rx[2].ena),
    .eth100g_rx_lbus_seg2_eop(lbus_rx[2].eop),
    .eth100g_rx_lbus_seg2_err(lbus_rx[2].err),
    .eth100g_rx_lbus_seg2_mty(lbus_rx[2].mty),
    .eth100g_rx_lbus_seg2_sop(lbus_rx[2].sop),
    .eth100g_rx_lbus_seg3_data(lbus_rx[3].data),
    .eth100g_rx_lbus_seg3_ena(lbus_rx[3].ena),
    .eth100g_rx_lbus_seg3_eop(lbus_rx[3].eop),
    .eth100g_rx_lbus_seg3_err(lbus_rx[3].err),
    .eth100g_rx_lbus_seg3_mty(lbus_rx[3].mty),
    .eth100g_rx_lbus_seg3_sop(lbus_rx[3].sop),
    .eth100g_tx_lbus_seg0_data(lbus_tx[0].data),
    .eth100g_tx_lbus_seg0_ena(lbus_tx[0].ena),
    .eth100g_tx_lbus_seg0_eop(lbus_tx[0].eop),
    .eth100g_tx_lbus_seg0_err(lbus_tx[0].err),
    .eth100g_tx_lbus_seg0_mty(lbus_tx[0].mty),
    .eth100g_tx_lbus_seg0_sop(lbus_tx[0].sop),
    .eth100g_tx_lbus_seg1_data(lbus_tx[1].data),
    .eth100g_tx_lbus_seg1_ena(lbus_tx[1].ena),
    .eth100g_tx_lbus_seg1_eop(lbus_tx[1].eop),
    .eth100g_tx_lbus_seg1_err(lbus_tx[1].err),
    .eth100g_tx_lbus_seg1_mty(lbus_tx[1].mty),
    .eth100g_tx_lbus_seg1_sop(lbus_tx[1].sop),
    .eth100g_tx_lbus_seg2_data(lbus_tx[2].data),
    .eth100g_tx_lbus_seg2_ena(lbus_tx[2].ena),
    .eth100g_tx_lbus_seg2_eop(lbus_tx[2].eop),
    .eth100g_tx_lbus_seg2_err(lbus_tx[2].err),
    .eth100g_tx_lbus_seg2_mty(lbus_tx[2].mty),
    .eth100g_tx_lbus_seg2_sop(lbus_tx[2].sop),
    .eth100g_tx_lbus_seg3_data(lbus_tx[3].data),
    .eth100g_tx_lbus_seg3_ena(lbus_tx[3].ena),
    .eth100g_tx_lbus_seg3_eop(lbus_tx[3].eop),
    .eth100g_tx_lbus_seg3_err(lbus_tx[3].err),
    .eth100g_tx_lbus_seg3_mty(lbus_tx[3].mty),
    .eth100g_tx_lbus_seg3_sop(lbus_tx[3].sop),
    .eth100g_tx_tx_rdyout(lbus_tx_rdyout));

endmodule
