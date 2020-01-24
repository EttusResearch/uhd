//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Arranges FFT output AXI stream packets so zero frequency bin is centered. Expects i_tuser to have FFT index.
// Intended to complement Xilinx Coregen AXI-stream FFT, but should work with any core with similar output.
// Works with natural and bit/digit reversed order. 
//
// When using Xilinx FFT core, use bit/digit reversed order (versus natural order) to save resources
//
// Config bits:
//   0: Reverse output so positive frequencies are sent first
//   1: Bypass fft shift

module fft_shift #(
  parameter MAX_FFT_SIZE_LOG2 = 11,
  parameter WIDTH = 32)
(
  input clk, input reset,
  input [1:0] config_tdata, input config_tvalid, output config_tready,
  input [$clog2(MAX_FFT_SIZE_LOG2+1)-1:0] fft_size_log2_tdata, input fft_size_log2_tvalid, output fft_size_log2_tready,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready, input [MAX_FFT_SIZE_LOG2-1:0] i_tuser,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  reg ping_pong;
  reg loading_pkt;
  reg [2:0] reconfig_stall;
  reg reverse, bypass;
  reg [$clog2(MAX_FFT_SIZE_LOG2+1)-1:0] fft_size_log2_reg;
  reg [MAX_FFT_SIZE_LOG2:0] fft_size;
  reg [MAX_FFT_SIZE_LOG2-1:0] fft_size_minus_1, fft_shift_mask;
  wire [WIDTH-1:0] ping_rd_data, pong_rd_data;
  reg [MAX_FFT_SIZE_LOG2-1:0] ping_rd_addr, pong_rd_addr;
  // t_user is the FFT index, this XOR is how the natural order FFT output is flipped to
  // center the zero frequency bin in the middle. This is essentially adding half the FFT length to
  // the write address without carrying, causing the upper half addresses to wrap around to the lower half
  // and vice versa.
  wire [MAX_FFT_SIZE_LOG2-1:0] ping_wr_addr = fft_shift_mask ^ i_tuser;
  wire [MAX_FFT_SIZE_LOG2-1:0] pong_wr_addr = fft_shift_mask ^ i_tuser;
  wire ping_wr_en = ping_pong ? i_tvalid & i_tready : 1'b0;
  wire pong_wr_en = ping_pong ? 1'b0                : i_tvalid & i_tready;
  // Always reads when loading ping/pong RAM so first word falls through. Avoids a bubble state.
  wire ping_rd_en = ping_pong ? 1'b1                : o_tvalid & o_tready;
  wire pong_rd_en = ping_pong ? o_tvalid & o_tready : 1'b1;
  reg ping_loaded, pong_loaded;
  // Only fill ping (or pong) RAM if it is empty and fft size has propagated through
  assign i_tready = (ping_pong ? ~ping_loaded       : ~pong_loaded) & ~reconfig_stall[2];
  reg ping_tlast, pong_tlast;
  // Dump data in ping RAM (but only if it has been loaded!) while also loading in pong RAM and vice versa
  assign o_tvalid = ping_pong ? pong_loaded         : ping_loaded;
  assign o_tlast = ping_pong ? pong_tlast           : ping_tlast;
  assign o_tdata = ping_pong ? pong_rd_data         : ping_rd_data;

  // Prevent reconfiguration from occurring except at valid times. If the user violates tvalid rules
  // (i.e. deasserts tvalid during the middle of a packet), could cause next output packet to have
  // the wrong size.
  assign config_tready        = ~ping_loaded & ~pong_loaded & ~loading_pkt;
  assign fft_size_log2_tready = config_tready;

  ram_2port #(
    .DWIDTH(WIDTH),
    .AWIDTH(MAX_FFT_SIZE_LOG2))
  ping_ram_2port (
    .clka(clk),.ena(1'b1),.wea(ping_wr_en),.addra(ping_wr_addr),.dia(i_tdata),.doa(),
    .clkb(clk),.enb(ping_rd_en),.web(1'b0),.addrb(ping_rd_addr),.dib({WIDTH{1'b0}}),.dob(ping_rd_data));

  ram_2port #(
    .DWIDTH(WIDTH),
    .AWIDTH(MAX_FFT_SIZE_LOG2))
  pong_ram_2port (
    .clka(clk),.ena(1'b1),.wea(pong_wr_en),.addra(pong_wr_addr),.dia(i_tdata),.doa(),
    .clkb(clk),.enb(pong_rd_en),.web(1'b0),.addrb(pong_rd_addr),.dib({WIDTH{1'b0}}),.dob(pong_rd_data));

  always @(posedge clk) begin
    if (reset) begin
      ping_pong             <= 1'b1;
      ping_loaded           <= 1'b0;
      pong_loaded           <= 1'b0;
      ping_rd_addr          <= 0;
      pong_rd_addr          <= 0;
      ping_tlast            <= 1'b0;
      pong_tlast            <= 1'b0;
      fft_shift_mask        <= 0;
      fft_size_minus_1      <= 0;
      fft_size              <= 0;
      fft_size_log2_reg     <= 0;
      bypass                <= 1'b0;
      reverse               <= 1'b0;
      reconfig_stall        <= 3'd0;
      loading_pkt           <= 1'b0;
    end else begin
      fft_size_minus_1   <= fft_size-1;
      fft_size           <= 1 << fft_size_log2_reg;
      // Configure FFT shift mask such that the output order is either
      // unaffected (bypass), positive frequencies first (reverse), or
      // negative frequencies first
      if (bypass) begin
        fft_shift_mask   <= 'd0;
      end else if (reverse) begin
        fft_shift_mask   <= (fft_size-1) >> 1;
      end else begin
        fft_shift_mask   <= fft_size >> 1;
      end

      // Restrict updating 
      if (config_tready & config_tvalid) begin
        reverse           <= config_tdata[0];
        bypass            <= config_tdata[1];
        reconfig_stall    <= 3'b100;
      end
      // Restrict updating FFT size to valid times
      // Also, deassert i_tready until updated fft size has propagated through
      if (fft_size_log2_tready & fft_size_log2_tvalid) begin
        fft_size_log2_reg <= fft_size_log2_tdata[$clog2(MAX_FFT_SIZE_LOG2)-1:0];
        reconfig_stall    <= 3'b111;
      end
      if (~(config_tready & config_tvalid) & ~(fft_size_log2_tready & fft_size_log2_tvalid)) begin
        reconfig_stall[0] <= 1'b0;
        reconfig_stall[2:1] <= reconfig_stall[1:0];
      end

      // Used to disable reconfiguration when we are receiving a packet
      if (i_tvalid & i_tready & ~i_tlast & ~loading_pkt) begin
        loading_pkt <= 1'b1;
      end else if (i_tvalid & i_tready & i_tlast & loading_pkt) begin
        loading_pkt <= 1'b0;
      end

      // Logic to simultaneously load ping RAM and unload pong RAM. Note, write address for ping RAM handled with i_tuser, 
      // so we only look for i_tlast instead of maintaining a write address counter.
      if (ping_pong) begin
        // Unload pong RAM
        if (pong_loaded & o_tready & o_tvalid) begin
          // i.e. pong_rd_addr == fft_size-1, more efficient to use tlast
          if (pong_tlast) begin
            // Special case: ping RAM loaded before pong RAM emptied
            if (ping_loaded | (i_tvalid & i_tready & i_tlast)) begin
              ping_pong <= ~ping_pong;
            end
            pong_tlast   <= 1'b0;
            pong_loaded  <= 1'b0;
            pong_rd_addr <= 0;
          end else begin
            pong_rd_addr <= pong_rd_addr + 1;
          end
          if (pong_rd_addr == fft_size_minus_1) begin
            pong_tlast <= 1'b1;
          end
        end
        // Ping RAM done loading
        if (i_tvalid & i_tready & i_tlast) begin
          // Value at addr 0 already loaded (see first word fall through and avoiding a bubble state comment above)
          ping_rd_addr <= 1;
          ping_loaded  <= 1'b1;
          // We can switch to the pong RAM only if it is empty (or about to be empty)
          if (~pong_loaded) begin
            ping_pong <= ~ping_pong;
          end
        end
        // Special case: Ping and pong RAM loaded, wait until pong RAM unloaded.
        if (ping_loaded & (pong_loaded & o_tvalid & o_tlast)) begin
          ping_pong <= ~ping_pong;
        end
      // Same as above, just ping / pong switched
      end else begin
        if (ping_loaded & o_tready & o_tvalid) begin
          if (ping_tlast) begin
            if (pong_loaded | (i_tvalid & i_tready & i_tlast)) begin
              ping_pong <= ~ping_pong;
            end
            ping_tlast   <= 1'b0;
            ping_loaded  <= 1'b0;
            ping_rd_addr <= 0;
          end else begin
            ping_rd_addr <= ping_rd_addr + 1;
          end
          if (ping_rd_addr == fft_size_minus_1) begin
            ping_tlast <= 1'b1;
          end
        end
        if (i_tvalid & i_tready & i_tlast) begin
          pong_rd_addr <= 1;
          pong_loaded  <= 1'b1;
          if (~ping_loaded | (ping_loaded & o_tvalid & o_tlast)) begin
            ping_pong <= ~ping_pong;
          end
        end
        if (pong_loaded & (ping_loaded & o_tvalid & o_tlast)) begin
          ping_pong <= ~ping_pong;
        end
      end
    end
  end

endmodule
