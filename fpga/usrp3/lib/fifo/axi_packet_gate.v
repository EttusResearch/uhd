//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//   Holds packets in a FIFO until they are complete. This allows buffering 
//   slowly-built packets so they don't clog up downstream logic. If o_tready
//   is held high, this module guarantees that o_tvalid will not be deasserted
//   until a full packet is transferred. This module can also optionally drop
//   a packet if the i_terror bit is asserted along with i_tlast. This allows
//   discarding packet, say, if a CRC check fails.
//   NOTE:
//   - The maximum size of a packet that can pass through this module is
//     2^SIZE lines. If a larger packet is sent, this module will lock up.
//   - Assuming that upstream is valid and downstream is ready, the maximum 
//     in to out latency per packet  is (2^SIZE + 2) clock cycles. 
//     2^SIZE because this module gates a packet, 1 cycle for the RAM read and
//     1 more cycle for the output register. This is not guaranteed behavior though.
//   - The USE_AS_BUFF parameter can be used to treat this packet gate as
//     a multi-packet buffer. When USE_AS_BUFF=0, the max number of packets 
//     (regardless of size) that the module can store is 2. When USE_AS_BUFF=1,
//     the entire storage of this module can be used to buffer packets but at
//     the cost of some additional RAM. Beware the sequence of (big packet,
//     small packet, small packet), as some outside buffering may be needed
//     to handle this case if USE_AS_BUFF=0.

module axi_packet_gate #(
  parameter WIDTH       = 64,   // Width of datapath
  parameter SIZE        = 10,   // log2 of the buffer size (must be >= MTU of packet)
  parameter USE_AS_BUFF = 0,    // Allow the packet gate to be used as a buffer (uses more RAM)
  parameter MIN_PKT_SIZE= 0     // log2 of minimum valid packet size (rounded down, used to reduce addr fifo size)
) (
  input  wire             clk, 
  input  wire             reset, 
  input  wire             clear,
  input  wire [WIDTH-1:0] i_tdata,
  input  wire             i_tlast,
  input  wire             i_terror,
  input  wire             i_tvalid,
  output wire             i_tready,
  output reg  [WIDTH-1:0] o_tdata = {WIDTH{1'b0}},
  output reg              o_tlast = 1'b0,
  output reg              o_tvalid = 1'b0,
  input  wire             o_tready
);

  localparam [SIZE-1:0] ADDR_ZERO = {SIZE{1'b0}};
  localparam [SIZE-1:0] ADDR_ONE  = {{(SIZE-1){1'b0}}, 1'b1};

  // -------------------------------------------
  // RAM block that will hold pkts
  // -------------------------------------------
  wire            wr_en, rd_en;
  wire [WIDTH:0]  wr_data, rd_data;
  reg  [SIZE-1:0] wr_addr = ADDR_ZERO, rd_addr = ADDR_ZERO;

  // Threshold to explicitly instantiate LUTRAM
  localparam LUTRAM_THRESH = 5;

  // We need to instantiate a simple dual-port RAM here so
  // we use the ram_2port module with one read port and one
  // write port and "NO-CHANGE" mode.
  ram_2port #(
    .DWIDTH (WIDTH+1), .AWIDTH(SIZE),
    .RW_MODE("NO-CHANGE"), .OUT_REG(0),
    .RAM_TYPE(SIZE <= LUTRAM_THRESH ? "LUTRAM" : "AUTOMATIC")
  ) ram_i (
    .clka (clk), .ena(1'b1), .wea(wr_en),
    .addra(wr_addr), .dia(wr_data), .doa(),
    .clkb (clk), .enb(rd_en), .web(1'b0),
    .addrb(rd_addr), .dib({WIDTH+1{1'b0}}), .dob(rd_data)
  );

  // FIFO empty/full logic. The condition for both
  // empty and full is when rd_addr == wr_addr. However,
  // it matters if we approach that case from the low side
  // or the high side. So keep track of the almost empty/full
  // state for determine if the next transaction will cause
  // the FIFO to be truly empty or full.
  reg  ram_full = 1'b0, ram_empty = 1'b1;
  wire almost_full  = (wr_addr == rd_addr - ADDR_ONE);
  wire almost_empty = (wr_addr == rd_addr + ADDR_ONE);

  always @(posedge clk) begin
    if (reset | clear) begin
      ram_full <= 1'b0;
    end else begin
      if (almost_full) begin
        if (wr_en & ~rd_en)
          ram_full <= 1'b1;
      end else begin
        if (~wr_en & rd_en)
          ram_full <= 1'b0;
      end
    end
  end

  always @(posedge clk) begin
    if (reset | clear) begin
      ram_empty <= 1'b1;
    end else begin
      if (almost_empty) begin
        if (rd_en & ~wr_en)
          ram_empty <= 1'b1;
      end else begin
        if (~rd_en & wr_en)
          ram_empty <= 1'b0;
      end
    end
  end

  // -------------------------------------------
  // Address FIFO
  // -------------------------------------------
  // The address FIFO will hold the write address
  // for the last line in a non-errant packet
  
  wire [SIZE-1:0] afifo_i_tdata, afifo_o_tdata, afifo_p_tdata;
  wire            afifo_i_tvalid, afifo_i_tready;
  wire            afifo_o_tvalid, afifo_o_tready;
  wire            afifo_p_tvalid, afifo_p_tready;

  axi_fifo #(.WIDTH(SIZE), .SIZE(USE_AS_BUFF==1 ? SIZE-MIN_PKT_SIZE : 1)) addr_fifo_i (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(afifo_i_tdata), .i_tvalid(afifo_i_tvalid), .i_tready(afifo_i_tready),
    .o_tdata(afifo_p_tdata), .o_tvalid(afifo_p_tvalid), .o_tready(afifo_p_tready),
    .space(), .occupied()
  );

  axi_fifo #(.WIDTH(SIZE), .SIZE(1)) addr_fifo_pipe_i (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(afifo_p_tdata), .i_tvalid(afifo_p_tvalid), .i_tready(afifo_p_tready),
    .o_tdata(afifo_o_tdata), .o_tvalid(afifo_o_tvalid), .o_tready(afifo_o_tready),
    .space(), .occupied()
  );

  // -------------------------------------------
  // Write state machine
  // -------------------------------------------
  reg  [SIZE-1:0] wr_head_addr  = ADDR_ZERO;

  assign i_tready = ~ram_full & afifo_i_tready;
  assign wr_en    = i_tvalid & i_tready;
  assign wr_data  = {i_tlast, i_tdata};

  always @(posedge clk) begin
    if (reset | clear) begin
      wr_addr <= ADDR_ZERO;
      wr_head_addr <= ADDR_ZERO;
    end else begin
      if (wr_en) begin
        if (i_tlast) begin
          if (i_terror) begin
            // Incoming packet had an error. Rewind the write
            // pointer and pretend that a packet never came in.
            wr_addr <= wr_head_addr;
          end else begin
            // Incoming packet had no error, advance wr_addr and
            // wr_head_addr for the next packet.
            wr_addr <= wr_addr + ADDR_ONE;
            wr_head_addr <= wr_addr + ADDR_ONE;
          end
        end else begin
          // Packet is still in progress, only update wr_addr
          wr_addr <= wr_addr + ADDR_ONE;
        end
      end
    end
  end

  // Push the write address to the address FIFO if
  // - It is the last one in the packet
  // - The packet has no errors
  assign afifo_i_tdata  = wr_addr;
  assign afifo_i_tvalid = ~ram_full & i_tvalid & i_tlast & ~i_terror;

  // -------------------------------------------
  // Read state machine
  // -------------------------------------------
  reg    rd_data_valid = 1'b0;
  wire   update_out_reg;
  // Data can be read if there is a valid last address in the
  // address FIFO (signifying the end of an input packet) and
  // if there is data available in RAM
  wire   ready_to_read = (~ram_empty) & afifo_o_tvalid;
  // Pop from address FIFO once we have see the end of the pkt
  assign afifo_o_tready = rd_en & (afifo_o_tdata == rd_addr);

  // Read from RAM if
  // - A full packet has been written AND
  // - Output data is not valid OR is currently being transferred
  assign rd_en = ready_to_read & (update_out_reg | ~rd_data_valid);

  always @(posedge clk) begin
    if (reset | clear) begin
      rd_data_valid <= 1'b0;
      rd_addr <= ADDR_ZERO;
    end else begin
      if (update_out_reg | ~rd_data_valid) begin
        // Output data is not valid OR is currently being transferred
        if (ready_to_read) begin
          rd_data_valid <= 1'b1;
          rd_addr <= rd_addr + ADDR_ONE;
        end else begin
          rd_data_valid <= 1'b0;  // Don't read
        end
      end
    end
  end

  // Instantiate an output register to break critical paths starting
  // at the RAM module. When ram_2port is inferred as BRAM, the tools
  // should absorb this register into the BRAM block without using
  // SLICE resources.
  always @(posedge clk) begin
    if (reset | clear) begin
      o_tvalid <= 1'b0;
    end else if (update_out_reg) begin
      o_tvalid <= rd_data_valid;
      {o_tlast, o_tdata} <= rd_data;
    end
  end
  // Update the output reg only *after* the downstream
  // block has consumed the current value
  assign update_out_reg = o_tready | ~o_tvalid;

endmodule
