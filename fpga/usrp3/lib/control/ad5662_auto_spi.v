//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// The AD5662 DAC serial interface uses 24-bit transfers to encode 16-bits
// of actual data, two bits for power-down mode, and six pad bits. This
// module stores a copy of the last-programmed value, and will generate a
// serial stream if ever the input word (dat) changes. It will ignore
// changes to (dat) while it is busy with a serial update.
//
module ad5662_auto_spi
(
  input clk,
  input [15:0] dat,
  output reg sclk,
  output reg mosi,
  output reg sync_n
);
  // initialize ldat to 0, thus forcing
  // a reload on init.
  reg [15:0] ldat = 16'd0;
  wire upd = (dat != ldat); // new data present, need to update hw

  reg [23:0] shft=24'b0;
  wire [23:0] nxt_shft;

  // clock cycle counter to throttle generated spi cycles
  // allowing one spi clock cycle every 16 cycles of clk, with clk at 200 MHz
  // gives a spi clock rate of 12 MHz. This can be made more sophisticated
  // or parameterized, if more flexibility in clk is needed, of course.
  reg [3:0] ccnt=4'b0;
  wire [3:0] nxt_ccnt = ccnt + 1'b1;
  wire half = ccnt==4'b1000;
  wire full = ccnt==4'b1111;
  reg sena, hena;
  wire cena;
  always @(posedge clk) if (cena) ccnt <= nxt_ccnt;
  always @(posedge clk) sena <= full; // state updates and rising sclk
  always @(posedge clk) hena <= half; // for falling sclk

  // transfer state counter
  reg [4:0] scnt = 5'b0;
  reg [4:0] nxt_scnt;
  always @(posedge clk) begin
    if (sena) begin
      scnt <= nxt_scnt;
      shft <= nxt_shft;
      mosi <= shft[23];
    end
  end


  // 32 possible states - more than enough to shift-out 24 bits and manage
  // the sync_n line

  // particular scnt values of interest
  localparam READY=5'b00000; // waiting for new data
  localparam DCAPT=5'b00001; // new data transfers into ldat
  localparam SYNCL=5'b00010; // assert sync_n low
  localparam SYNCH=5'b11011; // return sync_n high

  assign cena = upd | scnt != READY;

  always @(scnt or upd)
  begin
    case (scnt)
    READY:
      nxt_scnt = upd ? DCAPT : READY;
    SYNCH:
      nxt_scnt = READY;
    default:
      nxt_scnt = scnt + 1'b1;
    endcase
  end

  // note: defining the power-down mode bits to 00 for "normal operation"
  assign nxt_shft = (scnt == SYNCL) ? { 8'b000000_00, ldat } : { shft[22:0], 1'b0 };

  // Update ldat when dat has changed, but only if READY.
  // Changes to dat arriving faster than can be kept up with here are ignored
  // until the cycle-in-progress is completed.
  wire ldat_ena = sena & (scnt == DCAPT);
  always @(posedge clk) begin
    if (ldat_ena) ldat <= dat;
  end

  // keep the sync_n line low when idle to minimize power consumption
  // it gets brought high just before beginning each transaction
  wire nxt_sync_n = (scnt==SYNCL) | (scnt==SYNCH);
  always @(posedge clk) if (sena) sync_n <= nxt_sync_n;

  reg sclk_go;
  always @(posedge clk) sclk_go <= (scnt > SYNCL);
  wire nxt_sclk = ~sclk_go ? 1'b1 : ~sclk;
  always @(posedge clk) if (sena | hena) sclk <= nxt_sclk;

endmodule
