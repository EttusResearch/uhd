///////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rhodium_gain_ctrl
// Description:
//   Gain controller for Rhodium
//   Provides 2 SPI slaves:
//     The "load" slave is used to load the gain table with DSA settings for
//     each index.
//     The "ctrl" slave takes in a gain index and drives the DSAs with the
//     setting found in the gain table.
//   The SPI formats are provided below.
//////////////////////////////////////////////////////////////////////

`default_nettype none

/**
* SPI DATA FORMAT
*   LOADER
*     M {table_sel[1:0], gain_index[5:0], rsvd[1:0], dsa1[4:0], dsa2[4:0], wr_en, rsvd[2:0]}
*     S {-------------------------------, rsvd[1:0], dsa1[4:0], dsa2[4:0], -------rsvd[3:0]}
*   CTRL
*     M {table_sel[1:0], gain_index[5:0], rsvd[1:0], wr_dsa1, -rsvd[5:0], wr_dsa2, rsvd[5:0]}
*     S {-------------------------------, ---------rsvd[2:0], gain1[5:0], ---rsvd, gain2[5:0]}
*/
module rhodium_gain_ctrl
#(
  parameter TABLE_NUM = 2'b01
) (
  input  wire       load_table_sel,
  input  wire       load_sck,
  input  wire       load_csb,
  input  wire       load_mosi,
  output wire       load_miso,
  input  wire       ctrl_table_sel,
  input  wire       ctrl_sck,
  input  wire       ctrl_csb,
  input  wire       ctrl_mosi,
  output reg        ctrl_miso,
  output wire [4:0] dsa,
  output reg        dsa1_le,
  output reg        dsa2_le
);

localparam CNT_GAIN1_DRIVE = 10,
           CNT_DSA1_LE_RISE = 11,
           CNT_DSA1_LE_FALL = 14,
           CNT_GAIN1_RELEASE = 17;
localparam CNT_GAIN2_DRIVE = 17,
           CNT_DSA2_LE_RISE = 18,
           CNT_DSA2_LE_FALL = 21,
           CNT_GAIN2_RELEASE = 24;

/******************
* Gain table loader
*******************/
reg  [4:0]  load_bit_cnt;
reg  [1:0]  load_tbl;
reg  [5:0]  load_index;
reg  [15:0] load_rd_data;
reg  [4:0]  load_dsa1;
reg  [4:0]  load_dsa2;
wire [4:0]  load_dsa1_prev;
wire [4:0]  load_dsa2_prev;

assign load_miso = load_rd_data[15]; // Shift out on neg edge

wire wr_en;
assign wr_en = (!load_csb) && (load_tbl == TABLE_NUM) && (load_bit_cnt == 20) && (load_mosi);

// Cycle counter for where we are in protocol and shift register for input
always @ (posedge load_sck or posedge load_csb)
begin
  if (load_csb) begin
    load_bit_cnt <= 5'd0;
  end else if (!load_csb) begin
    {load_dsa1, load_dsa2} <= {load_dsa1[3:0], load_dsa2, load_mosi};

    if (load_bit_cnt < 23) begin
      load_bit_cnt <= load_bit_cnt + 5'd1;
    end
    if (load_bit_cnt < 8) begin
      {load_tbl, load_index} <= {load_tbl[0], load_index, load_mosi};
    end
  end
end

// Readback shift register
always @ (negedge load_sck)
begin
    if (load_bit_cnt == 9) begin
      load_rd_data <= {load_dsa1_prev, load_dsa2_prev, 5'b000};
    end else begin
      load_rd_data <= {load_rd_data[14:0], 1'b0};
    end
end

/******************
* Gain table RAM
*******************/
wire [4:0] ctrl_dsa1;
wire [4:0] ctrl_dsa2;

/* Use half of table for low band, other half for high band
 * Software decides address mapping
 */
rhodium_gain_table gain_table(
  .wr_clk(load_sck),
  .wr_en(wr_en),
  .wr_addr({load_table_sel, load_index}),
  .wr_data({load_dsa1, load_dsa2}),
  .wr_data_prev({load_dsa1_prev, load_dsa2_prev}),
  .rd_clk(ctrl_sck),
  .rd_addr({ctrl_table_sel, ctrl_index}),
  .rd_data({ctrl_dsa1, ctrl_dsa2})
);

/******************
* Gain control
*******************/
reg  [4:0]  ctrl_bit_cnt;
reg  [1:0]  ctrl_tbl;
reg  [5:0]  ctrl_index;

reg [5:0] gain1;
reg [5:0] gain2;
reg gain1_t;
reg gain2_t;

assign dsa = !gain1_t ? ctrl_dsa1 :
            (!gain2_t ? ctrl_dsa2 :
                        5'b11111);

// Cycle counter for where we are in protocol and shift register for input
// Also controls timing of DSAs' latch enable signals
always @ (posedge ctrl_sck or posedge ctrl_csb)
begin
  if (ctrl_csb) begin
    ctrl_bit_cnt <= 5'd0;
    dsa1_le      <= 1'b0;
    dsa2_le      <= 1'b0;
    gain1_t      <= 1'b1;
    gain2_t      <= 1'b1;
  end else if (!ctrl_csb) begin
    if (ctrl_bit_cnt < 23) begin
      ctrl_bit_cnt <= ctrl_bit_cnt + 5'd1;
    end

    if (ctrl_bit_cnt < 8) begin
      {ctrl_tbl, ctrl_index} <= {ctrl_tbl[0], ctrl_index, ctrl_mosi};
    end

    if (ctrl_tbl == TABLE_NUM) begin
      if ((ctrl_bit_cnt == CNT_GAIN1_DRIVE) && (ctrl_mosi)) begin
        gain1   <= ctrl_index;
        gain1_t <= 1'b0;
      end else if ((gain1_t == 1'b0) && (ctrl_bit_cnt == CNT_DSA1_LE_RISE)) begin
        dsa1_le <= 1'b1;
      end else if ((gain1_t == 1'b0) && (ctrl_bit_cnt == CNT_DSA1_LE_FALL)) begin
        dsa1_le <= 1'b0;
      end else if ((gain1_t == 1'b0) && (ctrl_bit_cnt == CNT_GAIN1_RELEASE)) begin
        gain1_t <= 1'b1;
      end

      if ((ctrl_bit_cnt == CNT_GAIN2_DRIVE) && (ctrl_mosi)) begin
        gain2   <= ctrl_index;
        gain2_t <= 1'b0;
      end else if ((gain2_t == 1'b0) && (ctrl_bit_cnt == CNT_DSA2_LE_RISE)) begin
        dsa2_le <= 1'b1;
      end else if ((gain2_t == 1'b0) && (ctrl_bit_cnt == CNT_DSA2_LE_FALL)) begin
        dsa2_le <= 1'b0;
      end else if ((gain2_t == 1'b0) && (ctrl_bit_cnt == CNT_GAIN2_RELEASE)) begin
        gain2_t <= 1'b1;
      end
    end
  end
end

// SPI readback for ctrl bus, based on current bit count
always @ (negedge ctrl_sck)
begin
  case (ctrl_bit_cnt) // Shift out on neg edge
  11:      ctrl_miso <= gain1[5];
  12:      ctrl_miso <= gain1[4];
  13:      ctrl_miso <= gain1[3];
  14:      ctrl_miso <= gain1[2];
  15:      ctrl_miso <= gain1[1];
  16:      ctrl_miso <= gain1[0];
  18:      ctrl_miso <= gain2[5];
  19:      ctrl_miso <= gain2[4];
  20:      ctrl_miso <= gain2[3];
  21:      ctrl_miso <= gain2[2];
  22:      ctrl_miso <= gain2[1];
  23:      ctrl_miso <= gain2[0];
  default: ctrl_miso <= 1'b0;
  endcase
end

endmodule // rhodium_gain_ctrl
`default_nettype wire
