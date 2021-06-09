//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_100g_axi2lbus
//
// Description:
//   Translate from AXI4S (Xilinx segmented ifc) to lbus.
//
//   Built using example provided from Xilinx
//
// Parameters:
//  - FIFO_DEPTH - FIFO will be 2** deep
//  - NUM_SEG    - Number of lbus segments coming in

import PkgEth100gLbus::*;

module eth_100g_axi2lbus #(
   parameter FIFO_DEPTH = 5,
   parameter NUM_SEG = 4
)
(

  // AXIS IF
  AxiStreamIf.slave  axis,

  // Lbus Segments

  input  logic  lbus_rdy,
  output lbus_t lbus_out [NUM_SEG-1:0]

);

  localparam SEG_DATA_WIDTH = DATA_WIDTH/NUM_SEG;
  localparam SEG_BYTES      = SEG_DATA_WIDTH/8;
  localparam SEG_MTY_WIDTH  = $clog2(SEG_BYTES);

  // post rotation lbus signals
  lbus_t lbus_d [NUM_SEG-1:0];

  //  Find last so we can find SOP
  logic found_last;

  // Propagate ready when asserting , propagate delayed ready while deasserting
  logic              axis_tready_i;
  assign axis.tready = axis_tready_i | lbus_rdy;

  always @(posedge axis.clk) begin
    axis_tready_i <= lbus_rdy;
  end

  //declare a segment width axis bus so I can use it's methods
  AxiStreamIf #(.DATA_WIDTH(SEG_DATA_WIDTH),.USER_WIDTH($clog2(SEG_BYTES)))
    seg_axi (axis.clk, axis.rst);
  assign seg_axi.tlast = 1'b1;

  logic [NUM_SEG:0] valid;
  assign valid[NUM_SEG] = 1'b0;

  genvar b;
  genvar s;
  generate begin : lbus_gen
    for (s=0; s < NUM_SEG; s=s+1) begin : segment_loop
      // Reverse data byte ordering on each segment
      for (b = 0; b < DATA_WIDTH/32; b=b+1) begin : byte_loop
        assign lbus_d[s].data[b*8 +: 8] = axis.tdata[((s+1)*(DATA_WIDTH/NUM_SEG)-8-(b*8)) +: 8];
      end : byte_loop
      // valid if tkeep is set for any bytes in the segment
      assign valid[s] =  (| axis.tkeep[s*SEG_BYTES +: SEG_BYTES]) & axis.tvalid;
      // enable when valid and transfering
      assign lbus_d[s].ena = valid[s] & axis.tready;
      // eop on last valid byte if last is set
      // we init an extra valid bit to 0 so if all valid bits for all segments are set we trigger an eop on the final segment
      assign lbus_d[s].eop = (valid[s] ^ valid[s+1]) & axis.tlast;
      // set error on all segmetns if tuser is set
      assign lbus_d[s].err = valid[s] & axis.tuser;
      // translate keep to trailing bytes and invert sign
      always_comb begin
        if (lbus_d[s].eop) begin
          lbus_d[s].mty = SEG_BYTES - seg_axi.keep2trailing(axis.tkeep[s*SEG_BYTES+: SEG_BYTES]);
        end else begin
          lbus_d[s].mty = 'b0;
        end
      end
      //SOP can only occur on segment 0, so init all the bits to zero, then assign segment 0
      if (s==0) begin
        assign lbus_d[s].sop = found_last & axis.tvalid & axis.tready;
      end else begin
        assign lbus_d[s].sop = 1'b0;
      end
      // assign the output DFF's
      always_ff @(posedge axis.clk) begin
        lbus_out[s].data <= lbus_d[s].data;
        lbus_out[s].ena  <= lbus_d[s].ena;
        lbus_out[s].sop  <= lbus_d[s].sop;
        lbus_out[s].eop  <= lbus_d[s].eop;
        lbus_out[s].err  <= lbus_d[s].err;
        lbus_out[s].mty  <= lbus_d[s].mty;
      end

    end : segment_loop
  end : lbus_gen
  endgenerate



  // SOP Statemachine
  always_ff @(posedge axis.clk) begin : sop_sm
   if(axis.rst) begin
      found_last <= 1'b1;
   end else begin
      if(axis.tvalid & axis.tlast & axis.tready) found_last <= 1'b1;
      else if(axis.tvalid & axis.tready)         found_last <= 1'b0;
    end
  end : sop_sm

endmodule


