//
// Copyright 2015 National Instruments
//

module file_sink #(
  parameter SR_SWAP_SAMPLES        = 0, // 0: Do nothing, 1: 8-bit swap, 2: 16-bit, 3: 32-bit
  parameter SR_ENDIANNESS          = 1, // 0: Do nothing, 1: 16-bit boundary, 2: 32-bit
  // Default (after reset) values for above settings register set to sc16 (reverse endianess on
  parameter DEFAULT_SWAP_SAMPLES   = 2,
  parameter DEFAULT_ENDIANNESS     = 2,
  parameter FILENAME = "")
(
  input clk_i,
  input rst_i,

  input set_stb_i,
  input [7:0] set_addr_i,
  input [31:0] set_data_i,

  input  [63:0] i_tdata,
  input  i_tlast,
  input  i_tvalid,
  output i_tready);

  integer file = 0;
  reg hdr = 1'b1;

  wire [1:0] swap_samples;
  wire [1:0] endianness;

  wire [63:0] data_int;
  wire [63:0] data;

  setting_reg #(
    .my_addr(SR_SWAP_SAMPLES),
    .width(2),
    .at_reset(DEFAULT_SWAP_SAMPLES))
  sr_swap_samples (
    .clk(clk_i),
    .rst(rst_i),
    .strobe(set_stb_i),
    .addr(set_addr_i),
    .in(set_data_i),
    .out(swap_samples),
    .changed());

  setting_reg #(
    .my_addr(SR_ENDIANNESS),
    .width(2),
    .at_reset(DEFAULT_ENDIANNESS))
  sr_endianness (
    .clk(clk_i),
    .rst(rst_i),
    .strobe(set_stb_i),
    .addr(set_addr_i),
    .in(set_data_i),
    .out(endianness),
    .changed());

  // We're ready as soon as the file is open
  assign i_tready = (file == 0) ? 1'b0 : 1'b1;

  // Swap samples
  assign data_int = (swap_samples == 2'd0) ?  i_tdata :
                    (swap_samples == 2'd1) ? {i_tdata[55:48], i_tdata[63:56], i_tdata[39:32], i_tdata[47:40],
                                              i_tdata[23:16], i_tdata[31:24], i_tdata[ 7: 0], i_tdata[15: 8]} :
                    (swap_samples == 2'd2) ? {i_tdata[47:32], i_tdata[63:48], i_tdata[15: 0], i_tdata[31:16]} :
                    (swap_samples == 2'd3) ? {i_tdata[31:0], i_tdata[63:32]} :
                                              64'd0;

  // Swap endianness
  assign data = (endianness == 2'd0) ?  data_int :
                (endianness == 2'd1) ? {data_int[47:32], data_int[63:48], data_int[15: 0], data_int[31:16]} :
                (endianness == 2'd2) ? {data_int[39:32], data_int[47:40], data_int[55:48], data_int[63:56],
                                        data_int[ 7: 0], data_int[15: 8], data_int[23:16], data_int[31:24]} :
                                        64'd0;

  initial begin
    if (FILENAME != "") begin
      file = $fopen(FILENAME, "wb");
      if(!file)
        $error("Could not open file sink.");
      $display("File sink ready.");
    end
  end

  always @(posedge clk_i) begin
    if(rst_i) begin
      hdr <= 1'b1;
    end
    else begin
      if(i_tvalid) begin
        if(hdr) begin
          hdr <= 1'b0;
        end
        else begin
          $fwrite(file, "%u", data);
        end
      end

      if(i_tlast) begin
        hdr <= 1'b1;
      end
    end
  end

endmodule
