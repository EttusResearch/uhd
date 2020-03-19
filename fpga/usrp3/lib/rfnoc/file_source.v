
// Copyright 2014, Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Dummy data source.  Turn it on by setting a packet length in its setting reg, turn it off by setting 0.  
// Will generate as fast as it can.

module file_source #(
  parameter SR_ENABLE              = 0,
  parameter SR_PKT_LENGTH          = 1,
  parameter SR_RATE                = 2,
  parameter SR_SEND_TIME           = 3,
  parameter SR_SWAP_SAMPLES        = 4, // 0: Do nothing, 1: 8-bit swap, 2: 16-bit, 3: 32-bit
  parameter SR_ENDIANNESS          = 5, // 0: Do nothing, 1: 16-bit boundary, 2: 32-bit
  // Default (after reset) values for above settings register set to sc16
  parameter DEFAULT_SWAP_SAMPLES   = 2,
  parameter DEFAULT_ENDIANNESS     = 2,
  parameter FILE_LENGTH            = 65536, // Bytes
  parameter FILENAME="")
(
  input clk, input reset, input [31:0] sid,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

  reg [63:0] mem[0:FILE_LENGTH/8-1];
  integer    file, file_length;
  reg [$clog2(FILE_LENGTH/8)-1:0] index;
  integer i;

  initial begin
    if (FILENAME != "") begin
      $readmemh(FILENAME, mem);
    end
  end

  reg  [11:0] seqnum;
  wire [15:0] rate;
  reg  [1:0]  state;
  reg  [15:0] line_number;

  wire [63:0] int_tdata;
  wire        int_tlast, int_tvalid, int_tready;

  wire        enable;
  wire [15:0] len;
  reg  [15:0] count;
  wire        send_time;
  wire [1:0]  swap_samples;
  wire [1:0]  endianness;

  setting_reg #(.my_addr(SR_ENABLE), .width(1)) sr_sid (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(enable), .changed());

  setting_reg #(.my_addr(SR_PKT_LENGTH), .width(16)) sr_len (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(len), .changed());

  setting_reg #(.my_addr(SR_RATE), .width(16)) sr_rate (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(rate), .changed());

  setting_reg #(.my_addr(SR_SEND_TIME), .width(1)) sr_send_time (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(send_time), .changed());

  setting_reg #(
    .my_addr(SR_SWAP_SAMPLES), .width(2), .at_reset(DEFAULT_SWAP_SAMPLES))
  sr_swap_samples (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(swap_samples), .changed());

  setting_reg #(
    .my_addr(SR_ENDIANNESS), .width(2), .at_reset(DEFAULT_ENDIANNESS))
  sr_endianness (
    .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
    .out(endianness), .changed());

  localparam IDLE = 2'd0;
  localparam HEAD = 2'd1;
  localparam TIME = 2'd2;
  localparam DATA = 2'd3;

  always @(posedge clk) begin
    if(reset) begin
      state <= IDLE;
      count <= 0;
      index <= 0;
      seqnum <= 0;
    end else begin
      case (state)
        IDLE : begin
          if (len != 0) begin
            state <= HEAD;
          end
        end
        HEAD : begin
          if (int_tvalid & int_tready) begin
            count <= 1;
            seqnum <= seqnum + 1;
            if (send_time) begin
              state <= TIME;
            end else begin
              state <= DATA;
            end
          end
        end
        TIME : begin
          if (int_tvalid & int_tready) begin
            state <= DATA;
          end
        end
        DATA : begin
          if (int_tvalid & int_tready) begin
            index <= index + 1;
            if (count == len) begin
              state <= IDLE;
              count <= 0;
            end else begin
              count <= count + 1;
            end
          end
        end
        default : state <= IDLE;
      endcase
    end
  end

  reg [63:0] time_cnt;
  always @(posedge clk) begin
    if (reset) begin
      time_cnt <= 'd0;
    end else begin
      time_cnt <= time_cnt + 1;
    end
  end

  wire [15:0] pkt_len = { len[12:0], 3'b000 } + 16'd8 + (send_time ? 16'd8 : 16'd0);

  wire [63:0] data_int = mem[index];
  // Swap endianness
  wire [63:0] data = (endianness == 2'd0) ?  data_int :
                     (endianness == 2'd1) ? {data_int[47:32], data_int[63:48], data_int[15: 0], data_int[31:16]} :
                     (endianness == 2'd2) ? {data_int[39:32], data_int[47:40], data_int[55:48], data_int[63:56],
                                             data_int[ 7: 0], data_int[15: 8], data_int[23:16], data_int[31:24]} :
                                             64'd0;
  // Swap samples
  wire [63:0] data_out = (swap_samples == 2'd0) ?  data :
                         (swap_samples == 2'd1) ? {data[55:48], data[63:56], data[39:32], data[47:40],
                                                   data[23:16], data[31:24], data[ 7: 0], data[15: 8]} :
                         (swap_samples == 2'd2) ? {data[47:32], data[63:48], data[15: 0], data[31:16]} :
                         (swap_samples == 2'd3) ? {data[31: 0], data[63:32]} :
                                                   64'd0;

  assign int_tdata = (state == HEAD) ? { 2'b00, send_time, 1'b0, seqnum, pkt_len, sid } :
                     (state == TIME) ? time_cnt : data_out;

  assign int_tlast = (count == len);

  reg [15:0]  line_timer;
  always @(posedge clk) begin
    if (reset) begin
       line_timer <= 0;
    end else begin
      if (line_timer == 0 || line_timer == 1) begin
        line_timer <= rate;
      end else begin
        line_timer <= line_timer - 1;
      end
    end
  end

  assign int_tvalid = enable & ((state==HEAD)|(state==DATA)|(state==TIME)) & (line_timer==0 || line_timer==1);

  axi_packet_gate #(.WIDTH(64)) gate (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i_tdata(int_tdata), .i_tlast(int_tlast), .i_terror(1'b0), .i_tvalid(int_tvalid), .i_tready(int_tready),
    .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

endmodule // file_source
