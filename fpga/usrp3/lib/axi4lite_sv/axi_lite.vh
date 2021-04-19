//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Header File: axi_lite.vh
// Description: Macros for use with AXI4-Lite
//

//-----------------------------------------------------------------------------
// AXI4-Lite
//-----------------------------------------------------------------------------

// Macro that drives o from i for all fields. Of course ready runs in the
// counter direction.
`define AXI4LITE_ASSIGN(O,I) \
  /* write address channel */\
  ``O.awaddr  = ``I.awaddr;\
  ``O.awvalid = ``I.awvalid;\
  ``I.awready = ``O.awready;\
  /* write data channel */\
  ``O.wdata   = ``I.wdata;\
  ``O.wstrb   = ``I.wstrb;\
  ``O.wvalid  = ``I.wvalid;\
  ``I.wready  = ``O.wready;\
  /* write resp channel */\
  ``I.bresp   = ``O.bresp;\
  ``I.bvalid  = ``O.bvalid;\
  ``O.bready  = ``I.bready;\
  /* read address channel */\
  ``O.araddr  = ``I.araddr;\
  ``O.arvalid = ``I.arvalid;\
  ``I.arready = ``O.arready;\
  /* read resp channel */\
  ``I.rdata   = ``O.rdata;\
  ``I.rresp   = ``O.rresp;\
  ``I.rvalid  = ``O.rvalid;\
  ``O.rready  = ``I.rready;

// All signals
`define AXI4LITE_PORT_ASSIGN(FORMAL,ACTUAL) \
  .``FORMAL``_aclk(``ACTUAL``.clk),\
  .``FORMAL``_sreset(``ACTUAL``.rst),\
  .``FORMAL``_araddr(``ACTUAL``.araddr),\
  .``FORMAL``_arready(``ACTUAL``.arready),\
  .``FORMAL``_arvalid(``ACTUAL``.arvalid),\
  .``FORMAL``_awaddr(``ACTUAL``.awaddr),\
  .``FORMAL``_awready(``ACTUAL``.awready),\
  .``FORMAL``_awvalid(``ACTUAL``.awvalid),\
  .``FORMAL``_bready(``ACTUAL``.bready),\
  .``FORMAL``_bresp(``ACTUAL``.bresp[1:0]),\
  .``FORMAL``_bvalid(``ACTUAL``.bvalid),\
  .``FORMAL``_rdata(``ACTUAL``.rdata),\
  .``FORMAL``_rready(``ACTUAL``.rready),\
  .``FORMAL``_rresp(``ACTUAL``.rresp[1:0]),\
  .``FORMAL``_rvalid(``ACTUAL``.rvalid),\
  .``FORMAL``_wdata(``ACTUAL``.wdata),\
  .``FORMAL``_wready(``ACTUAL``.wready),\
  .``FORMAL``_wstrb(``ACTUAL``.wstrb),\
  .``FORMAL``_wvalid(``ACTUAL``.wvalid),

//NR removes clock and reset
`define AXI4LITE_PORT_ASSIGN_NR(FORMAL,ACTUAL) \
  .``FORMAL``_araddr(``ACTUAL``.araddr),\
  .``FORMAL``_arready(``ACTUAL``.arready),\
  .``FORMAL``_arvalid(``ACTUAL``.arvalid),\
  .``FORMAL``_awaddr(``ACTUAL``.awaddr),\
  .``FORMAL``_awready(``ACTUAL``.awready),\
  .``FORMAL``_awvalid(``ACTUAL``.awvalid),\
  .``FORMAL``_bready(``ACTUAL``.bready),\
  .``FORMAL``_bresp(``ACTUAL``.bresp[1:0]),\
  .``FORMAL``_bvalid(``ACTUAL``.bvalid),\
  .``FORMAL``_rdata(``ACTUAL``.rdata),\
  .``FORMAL``_rready(``ACTUAL``.rready),\
  .``FORMAL``_rresp(``ACTUAL``.rresp[1:0]),\
  .``FORMAL``_rvalid(``ACTUAL``.rvalid),\
  .``FORMAL``_wdata(``ACTUAL``.wdata),\
  .``FORMAL``_wready(``ACTUAL``.wready),\
  .``FORMAL``_wstrb(``ACTUAL``.wstrb),\
  .``FORMAL``_wvalid(``ACTUAL``.wvalid),

//NRS removes wstrb clock and reset
`define AXI4LITE_PORT_ASSIGN_NRS(FORMAL,ACTUAL) \
  .``FORMAL``_araddr(``ACTUAL``.araddr),\
  .``FORMAL``_arready(``ACTUAL``.arready),\
  .``FORMAL``_arvalid(``ACTUAL``.arvalid),\
  .``FORMAL``_awaddr(``ACTUAL``.awaddr),\
  .``FORMAL``_awready(``ACTUAL``.awready),\
  .``FORMAL``_awvalid(``ACTUAL``.awvalid),\
  .``FORMAL``_bready(``ACTUAL``.bready),\
  .``FORMAL``_bresp(``ACTUAL``.bresp[1:0]),\
  .``FORMAL``_bvalid(``ACTUAL``.bvalid),\
  .``FORMAL``_rdata(``ACTUAL``.rdata),\
  .``FORMAL``_rready(``ACTUAL``.rready),\
  .``FORMAL``_rresp(``ACTUAL``.rresp[1:0]),\
  .``FORMAL``_rvalid(``ACTUAL``.rvalid),\
  .``FORMAL``_wdata(``ACTUAL``.wdata),\
  .``FORMAL``_wready(``ACTUAL``.wready),\
  .``FORMAL``_wvalid(``ACTUAL``.wvalid),

`define AXI4LITE_DEBUG_ASSIGN(O,I) \
  (* mark_debug = "true" *) logic [``I.ADDR_WIDTH-1:0]     ``I``_debug_awaddr;\
  (* mark_debug = "true" *) logic                          ``I``_debug_awvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_awready;\
  (* mark_debug = "true" *) logic [``I.DATA_WIDTH-1:0]     ``I``_debug_wdata;\
  (* mark_debug = "true" *) logic [``I.BYTES_PER_WORD-1:0] ``I``_debug_wstrb;\
  (* mark_debug = "true" *) logic                          ``I``_debug_wvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_wready;\
  (* mark_debug = "true" *) logic [1:0]                    ``I``_debug_bresp;\
  (* mark_debug = "true" *) logic                          ``I``_debug_bvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_bready;\
  (* mark_debug = "true" *) logic [``I.ADDR_WIDTH-1:0]     ``I``_debug_araddr;\
  (* mark_debug = "true" *) logic                          ``I``_debug_arvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_arready;\
  (* mark_debug = "true" *) logic [``I.DATA_WIDTH-1:0]     ``I``_debug_rdata;\
  (* mark_debug = "true" *) logic [1:0]                    ``I``_debug_rresp;\
  (* mark_debug = "true" *) logic                          ``I``_debug_rvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_rready;\
  always_comb begin\
  /* write address channel */\
  ``I``_debug_awaddr  = ``I.awaddr;\
  ``I``_debug_awvalid = ``I.awvalid;\
  ``I.awready = ``I``_debug_awready;\
  /* write data channel */\
  ``I``_debug_wdata   = ``I.wdata;\
  ``I``_debug_wstrb   = ``I.wstrb;\
  ``I``_debug_wvalid  = ``I.wvalid;\
  ``I.wready  = ``I``_debug_wready;\
  /* write resp channel */\
  ``I.bresp   = ``I``_debug_bresp;\
  ``I.bvalid  = ``I``_debug_bvalid;\
  ``I``_debug_bready  = ``I.bready;\
  /* read address channel */\
  ``I``_debug_araddr  = ``I.araddr;\
  ``I``_debug_arvalid = ``I.arvalid;\
  ``I.arready = ``I``_debug_arready;\
  /* read resp channel */\
  ``I.rdata   = ``I``_debug_rdata;\
  ``I.rresp   = ``I``_debug_rresp;\
  ``I.rvalid  = ``I``_debug_rvalid;\
  ``I``_debug_rready  = ``I.rready;\
  end\
  always_comb begin\
  /* write address channel */\
  ``O.awaddr  = ``I``_debug_awaddr;\
  ``O.awvalid = ``I``_debug_awvalid;\
  ``I``_debug_awready = ``O.awready;\
  /* write data channel */\
  ``O.wdata   = ``I``_debug_wdata;\
  ``O.wstrb   = ``I``_debug_wstrb;\
  ``O.wvalid  = ``I``_debug_wvalid;\
  ``I``_debug_wready  = ``O.wready;\
  /* write resp channel */\
  ``I``_debug_bresp   = ``O.bresp;\
  ``I``_debug_bvalid  = ``O.bvalid;\
  ``O.bready  = ``I``_debug_bready;\
  /* read address channel */\
  ``O.araddr  = ``I``_debug_araddr;\
  ``O.arvalid = ``I``_debug_arvalid;\
  ``I``_debug_arready = ``O.arready;\
  /* read resp channel */\
  ``I``_debug_rdata   = ``O.rdata;\
  ``I``_debug_rresp   = ``O.rresp;\
  ``I``_debug_rvalid  = ``O.rvalid;\
  ``O.rready  = ``I``_debug_rready;\
  end
