//
// Copyright 2016 Ettus Research LLC
//

module aurora_axis_mac #(
   parameter PHY_ENDIANNESS   = "LITTLE", //{"LITTLE, "BIG"}
   parameter PACKET_MODE      = 0,
   parameter MAX_PACKET_SIZE  = 512,
   parameter BIST_ENABLED     = 1
) (
   // Clocks and resets
   input             phy_clk,
   input             phy_rst,
   input             sys_clk,
   input             sys_rst,
   input             clear,
   // PHY TX Interface (Synchronous to phy_clk)
   output [63:0]     phy_m_axis_tdata,
   output            phy_m_axis_tvalid,
   input             phy_m_axis_tready,
   // PHY RX Interface (Synchronous to phy_clk)
   input  [63:0]     phy_s_axis_tdata,
   input             phy_s_axis_tvalid,
   // User TX Interface (Synchronous to sys_clk)
   input  [63:0]     s_axis_tdata,
   input             s_axis_tlast,
   input             s_axis_tvalid,
   output            s_axis_tready,
   // User RX Interface (Synchronous to sys_clk)
   output [63:0]     m_axis_tdata,
   output            m_axis_tlast,
   output            m_axis_tvalid,
   input             m_axis_tready,
   // PHY Status Inputs (Synchronous to phy_clk)
   input             channel_up,
   input             hard_err,
   input             soft_err,
   // Status and Error Outputs (Synchronous to sys_clk)
   output [31:0]     overruns,
   output [31:0]     soft_errors,
   output reg [31:0] checksum_errors,
   output critical_err,
   // BIST Interface (Synchronous to sys_clk)
   input             bist_gen_en,
   input  [5:0]      bist_gen_rate,
   input             bist_checker_en,
   input             bist_loopback_en,
   output reg        bist_checker_locked,
   output reg [47:0] bist_checker_samps,
   output reg [47:0] bist_checker_errors
);

   // ----------------------------------------------
   // Resets, Clears, Clock crossings
   // ----------------------------------------------

   wire phy_s_axis_tready;    // Internal only. The PHY has no backpressure signal.

   // Stay idle if the PHY is not up or if it experiences a fatal error 
   wire clear_sysclk, clear_phyclk;
   synchronizer #(.INITIAL_VAL(1'b1)) clear_sync_phyclk_i (
      .clk(phy_clk), .rst(1'b0 /* no reset */), .in((~channel_up) | hard_err | clear), .out(clear_phyclk));
   synchronizer #(.INITIAL_VAL(1'b1)) clear_sync_sysclk_i (
      .clk(sys_clk), .rst(1'b0 /* no reset */), .in(clear_phyclk), .out(clear_sysclk));

   // ----------------------------------------------
   // Counters
   // ----------------------------------------------

   reg [31:0] overruns_reg;
   reg [31:0] soft_errors_reg;

   // Counter for recoverable errors. For reporting only.
   always @(posedge phy_clk)
      if (phy_rst | clear_phyclk)
         soft_errors_reg <= 32'd0;
      else if (soft_err)
         soft_errors_reg <= soft_errors_reg + 32'd1;

   // Tag an overrun if the FIFO is full. Samples will get dropped
   always @(posedge phy_clk)
      if (phy_rst | clear_phyclk)
         overruns_reg <= 32'd0;
      else if (phy_s_axis_tvalid & ~phy_s_axis_tready)
         overruns_reg <= overruns_reg + 32'd1;

   wire [7:0] dummy0;
   fifo_short_2clk status_counters_2clk_i (
      .rst(phy_rst),
      .wr_clk(phy_clk), .din({8'h00, soft_errors_reg, overruns_reg}), .wr_en(1'b1), .full(), .wr_data_count(),
      .rd_clk(sys_clk), .dout({dummy0, soft_errors, overruns}), .rd_en(1'b1), .empty(), .rd_data_count()
   );

   // ----------------------------------------------
   // BIST Wires
   // ----------------------------------------------

   wire [63:0] bist_o_tdata;
   wire        bist_o_tvalid, bist_o_tready;
   wire [63:0] bist_i_tdata;
   wire        bist_i_tvalid, bist_i_tready;
   wire [63:0] loopback_tdata;
   wire        loopback_tvalid, loopback_tready;
   reg         bist_gen_en_reg = 1'b0, bist_checker_en_reg = 1'b0, bist_loopback_en_reg = 1'b0;
   reg  [5:0]  bist_gen_rate_reg = 'd0;

   generate if (BIST_ENABLED == 1) begin
      // Pipeline control signals
      always @(posedge sys_clk) begin
         if (sys_rst | clear_sysclk) begin
            bist_gen_en_reg      <= 1'b0;
            bist_checker_en_reg  <= 1'b0;
            bist_loopback_en_reg <= 1'b0;
            bist_gen_rate_reg    <= 'd0;
         end else begin
            bist_gen_en_reg      <= bist_gen_en;
            bist_checker_en_reg  <= bist_checker_en;
            bist_loopback_en_reg <= bist_loopback_en;
            bist_gen_rate_reg    <= bist_gen_rate;
         end
      end
   end endgenerate
   // ----------------------------------------------
   // RX Data Path
   // ----------------------------------------------

   wire [63:0]    i_raw_tdata;
   wire           i_raw_tvalid, i_raw_tready;

   wire [63:0]    i_pip_tdata;
   wire           i_pip_tvalid, i_pip_tready;

   wire [63:0]    i_pkt_tdata;
   wire           i_pkt_tlast, i_pkt_tvalid, i_pkt_tready;

   wire [63:0]    i_gt_tdata;
   wire           i_gt_tlast, i_gt_tvalid, i_gt_tready;

   wire           checksum_err;

   wire [63:0]    phy_s_axis_tdata_endian, phy_m_axis_tdata_endian;
   
   generate if (PHY_ENDIANNESS == "BIG") begin
      assign phy_s_axis_tdata_endian = {
         phy_s_axis_tdata[7:0], phy_s_axis_tdata[15:8], phy_s_axis_tdata[23:16], phy_s_axis_tdata[31:24],
         phy_s_axis_tdata[39:32], phy_s_axis_tdata[47:40], phy_s_axis_tdata[55:48], phy_s_axis_tdata[63:56]
      };
      assign phy_m_axis_tdata = {
         phy_m_axis_tdata_endian[7:0], phy_m_axis_tdata_endian[15:8], phy_m_axis_tdata_endian[23:16], phy_m_axis_tdata_endian[31:24],
         phy_m_axis_tdata_endian[39:32], phy_m_axis_tdata_endian[47:40], phy_m_axis_tdata_endian[55:48], phy_m_axis_tdata_endian[63:56]
      };
   end else begin
      assign phy_s_axis_tdata_endian = phy_s_axis_tdata;
      assign phy_m_axis_tdata = phy_m_axis_tdata_endian;
   end endgenerate

   // Large FIFO must be able to run input side at 64b@156MHz to sustain 10Gb Rx.
   axi64_4k_2clk_fifo ingress_fifo_i (
      .s_aresetn(~phy_rst), .s_aclk(phy_clk),
      .s_axis_tdata(phy_s_axis_tdata_endian), .s_axis_tlast(phy_s_axis_tvalid), .s_axis_tuser(4'h0),
      .s_axis_tvalid(phy_s_axis_tvalid), .s_axis_tready(phy_s_axis_tready), .axis_wr_data_count(),
      .m_aclk(sys_clk),
      .m_axis_tdata(i_raw_tdata), .m_axis_tlast(), .m_axis_tuser(),
      .m_axis_tvalid(i_raw_tvalid), .m_axis_tready(i_raw_tready), .axis_rd_data_count()
   );

   // AXI-Flop to ease timing
   axi_fifo_flop #(.WIDTH(64)) input_pipe_i0 (
      .clk(sys_clk), .reset(sys_rst), .clear(clear_sysclk),
      .i_tdata(i_raw_tdata), .i_tvalid(i_raw_tvalid), .i_tready(i_raw_tready),
      .o_tdata(i_pip_tdata), .o_tvalid(i_pip_tvalid),
      .o_tready(bist_checker_en_reg ? bist_i_tready : (bist_loopback_en_reg ? loopback_tready : i_pip_tready)),
      .space(), .occupied()
   );

   assign bist_i_tdata     = i_pip_tdata;
   assign bist_i_tvalid    = i_pip_tvalid & bist_checker_en_reg;

   assign loopback_tdata   = i_pip_tdata;
   assign loopback_tvalid  = i_pip_tvalid & bist_loopback_en_reg;
   
   axi_strip_preamble #(.WIDTH(64), .MAX_PKT_SIZE(MAX_PACKET_SIZE)) axi_strip_preamble_i (
      .clk(sys_clk), .reset(sys_rst), .clear(clear_sysclk),
      .i_tdata(i_pip_tdata), .i_tvalid(i_pip_tvalid & ~bist_checker_en_reg & ~bist_loopback_en_reg), .i_tready(i_pip_tready),
      .o_tdata(i_gt_tdata), .o_tlast(i_gt_tlast), .o_tvalid(i_gt_tvalid), .o_tready(i_gt_tready),
      .crc_err(checksum_err), .pkt_dropped(), .crit_error(critical_err)
   );

  axi_fifo_flop #(.WIDTH(65)) input_pipe_i1 (
     .clk(sys_clk), .reset(sys_rst), .clear(clear_sysclk),
     .i_tdata({i_gt_tlast, i_gt_tdata}), .i_tvalid(i_gt_tvalid), .i_tready(i_gt_tready),
     .o_tdata({m_axis_tlast, m_axis_tdata}), .o_tvalid(m_axis_tvalid), .o_tready(m_axis_tready),
     .space(), .occupied()
  );

   always @(posedge sys_clk)
      if (sys_rst | clear_sysclk)
         checksum_errors <= 32'd0;
      else if (checksum_err)
         checksum_errors <= checksum_errors + 32'd1;

   // ----------------------------------------------
   // TX Data Path
   // ----------------------------------------------

   wire [63:0]    o_pkt_tdata;
   wire           o_pkt_tlast, o_pkt_tvalid, o_pkt_tready;

   wire [63:0]    o_pip_tdata;
   wire           o_pip_tvalid, o_pip_tready;

   wire [63:0]    o_raw_tdata;
   wire           o_raw_tvalid, o_raw_tready;

   // AXI-Flop to ease timing
   axi_fifo_flop #(.WIDTH(65)) output_pipe_i0 (
      .clk(sys_clk), .reset(sys_rst), .clear(clear_sysclk),
      .i_tdata({s_axis_tlast, s_axis_tdata}), .i_tvalid(s_axis_tvalid), .i_tready(s_axis_tready),
      .o_tdata({o_pkt_tlast, o_pkt_tdata}), .o_tvalid(o_pkt_tvalid), .o_tready(o_pkt_tready),
      .space(), .occupied()
   );

   // Insert preamble and EOP
   axi_add_preamble #(.WIDTH(64)) axi_add_preamble_i (
      .clk(sys_clk), .reset(sys_rst), .clear(clear_sysclk),
      .i_tdata(o_pkt_tdata), .i_tlast(o_pkt_tlast), .i_tvalid(o_pkt_tvalid), .i_tready(o_pkt_tready),
      .o_tdata(o_pip_tdata), .o_tvalid(o_pip_tvalid), .o_tready(o_pip_tready & ~bist_gen_en_reg & ~bist_loopback_en_reg)
   );

   // AXI-Flop to ease timing
   axi_fifo_flop #(.WIDTH(64)) output_pipe_i1 (
      .clk(sys_clk), .reset(sys_rst), .clear(clear_sysclk),
      .i_tdata(bist_gen_en_reg ? bist_o_tdata : (bist_loopback_en_reg ? loopback_tdata : o_pip_tdata)),
      .i_tvalid(bist_gen_en_reg ? bist_o_tvalid : (bist_loopback_en_reg ? loopback_tvalid : o_pip_tvalid)),
      .i_tready(o_pip_tready),
      .o_tdata(o_raw_tdata), .o_tvalid(o_raw_tvalid), .o_tready(o_raw_tready),
      .space(), .occupied()
   );

   assign bist_o_tready    = o_pip_tready;
   assign loopback_tready  = o_pip_tready;

   // Egress FIFO
   axi64_4k_2clk_fifo egress_fifo_i (
      .s_aresetn(~phy_rst), .s_aclk(sys_clk),
      .s_axis_tdata(o_raw_tdata), .s_axis_tlast(o_raw_tvalid), .s_axis_tuser(4'h0),
      .s_axis_tvalid(o_raw_tvalid), .s_axis_tready(o_raw_tready), .axis_wr_data_count(),
      .m_aclk(phy_clk),
      .m_axis_tdata(phy_m_axis_tdata_endian), .m_axis_tlast(), .m_axis_tuser(),
      .m_axis_tvalid(phy_m_axis_tvalid), .m_axis_tready(phy_m_axis_tready), .axis_rd_data_count()
   );

   // -------------------------------------------------
   // BIST: Generator and checker for a LFSR polynomial
   // -------------------------------------------------
   localparam LFSR_LEN  = 32;
   localparam LFSR_SEED = {LFSR_LEN{1'b1}};
   
   function [LFSR_LEN-1:0] compute_lfsr_next;
      input [LFSR_LEN-1:0] current;
      // Maximal length polynomial: x^32 + x^22 + x^2 + x^1 + 1
      compute_lfsr_next = {current[30:0], current[31]^current[21]^current[1]^current[0]};
   endfunction

   function [63:0] lfsr_to_axis;
      input [LFSR_LEN-1:0] lfsr;
      lfsr_to_axis = {~lfsr, lfsr};
   endfunction

   function [LFSR_LEN-1:0] axis_to_lfsr;
      input [63:0] axis;
      axis_to_lfsr = axis[LFSR_LEN-1:0];
   endfunction

   generate if (BIST_ENABLED == 1) begin
      // Throttle outgoing LFSR to based on the specified rate
      // BIST Throughput = sys_clk BW * (bist_gen_rate+1)/64
      reg [5:0] throttle_cnt;
      always @(posedge sys_clk) begin
         if (sys_rst | clear_sysclk)
            throttle_cnt <= 6'd0;
         else if (bist_gen_en_reg)
            throttle_cnt <= throttle_cnt + 6'd1;
      end
      // NOTE: This techinically violates AXIS spec (valid revocation)
      assign bist_o_tvalid = bist_gen_en_reg && (throttle_cnt <= bist_gen_rate_reg);

      // Unsynchronized LFSR generator (for BIST output)
      reg [LFSR_LEN-1:0] lfsr_gen = LFSR_SEED, lfsr_check = LFSR_SEED;
      always @(posedge sys_clk) begin
         if (sys_rst | clear_sysclk | ~bist_gen_en_reg)
            lfsr_gen <= LFSR_SEED;
         else if (bist_o_tready & bist_o_tvalid)
            lfsr_gen <= compute_lfsr_next(lfsr_gen);
      end
      assign bist_o_tdata = lfsr_to_axis(lfsr_gen);

      // Synchronized LFSR checker (for BIST input)
      wire [LFSR_LEN-1:0] lfsr_next = compute_lfsr_next(lfsr_check);;
      always @(posedge sys_clk) begin
         if (sys_rst | clear_sysclk | ~bist_checker_en_reg) begin
            bist_checker_locked <= 1'b0;
            lfsr_check <= LFSR_SEED;
         end else if (bist_i_tvalid && bist_i_tready) begin
            lfsr_check <= axis_to_lfsr(bist_i_tdata);
            if (bist_i_tdata == lfsr_to_axis(LFSR_SEED))
               bist_checker_locked <= 1'b1;
         end
      end

      // LFSR checker
      always @(posedge sys_clk) begin
         if (bist_checker_locked) begin
            if (bist_i_tvalid & bist_i_tready) begin
               bist_checker_samps <= bist_checker_samps + 48'd1;
               if (bist_i_tdata != lfsr_to_axis(lfsr_next)) begin
                  bist_checker_errors <= bist_checker_errors + 48'd1;
               end
            end
         end else begin
            bist_checker_samps  <= 48'd0;
            bist_checker_errors <= 48'd0;
         end
      end
      assign bist_i_tready = 1'b1;
   end endgenerate

endmodule

