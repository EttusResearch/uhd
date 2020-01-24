//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_serializer #(
  parameter WIDTH = 32)
(
  input clk, input rst, input reverse_input,
  input [WIDTH-1:0] i_tdata, input  i_tlast, input i_tvalid,  output reg i_tready,
  output reg o_tdata, output reg o_tlast, output reg o_tvalid, input o_tready
);

  reg i_tlast_latch;
  reg [WIDTH-1:0] serial_data_reg;
  reg [$clog2(WIDTH)-1:0] serial_cnt;
  reg serializing;

  always @(posedge clk) begin
    if (rst) begin
      i_tready        <= 1'b0;
      i_tlast_latch   <= 1'b0;
      o_tdata         <= 1'b0;
      o_tlast         <= 1'b0;
      o_tvalid        <= 1'b0;
      serial_data_reg <= 'd0;
      serializing     <= 1'b0;
      serial_cnt      <= 0;
    end else begin
      i_tready <= 1'b0;
      // Shift out a bit when downstream can consume it
      if (serializing & o_tready) begin
        o_tvalid <= 1'b1;
        if (reverse_input) begin
          o_tdata                    <= serial_data_reg[0];
          serial_data_reg[WIDTH-2:0] <= serial_data_reg[WIDTH-1:1];
        end else begin
          o_tdata                    <= serial_data_reg[WIDTH-1];
          serial_data_reg[WIDTH-1:1] <= serial_data_reg[WIDTH-2:0];
        end
        if (serial_cnt == WIDTH-1) begin
          serial_cnt      <= 0;
          serial_data_reg <= i_tdata;
          i_tlast_latch   <= i_tlast;
          o_tlast         <= i_tlast_latch;
          if (~i_tvalid) begin
            serializing <= 1'b0;
          end else begin
            i_tready <= 1'b1;
          end
        end else begin
          serial_cnt <= serial_cnt + 1;
        end
      end else if (~serializing) begin
        i_tready <= 1'b1;
        if (o_tvalid && o_tready) begin
          o_tvalid <= 1'b0;
        end
        // Serial shift register (serial_data_reg) is empty, load it 
        if (i_tvalid) begin
          i_tready        <= 1'b0;
          serializing     <= 1'b1;
          i_tlast_latch   <= i_tlast;
          serial_data_reg <= i_tdata;
        end
      end
    end
  end

endmodule