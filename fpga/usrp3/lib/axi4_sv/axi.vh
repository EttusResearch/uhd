//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Header File: axi.vh
//
// Description: Macros for use with AxiIf (AXI4 full)
//

//-----------------------------------------------------------------------------
// AXI4 (FULL)
//-----------------------------------------------------------------------------
//
// Difference between AXILITE and AXI(FULL)
//  Address channels contain
//    LEN, SIZE, BURST, LOCK, CACHE, PROT, QOS
//  Data Channels contain
//    LAST

// Macro that drives o from i for all fields. Of course ready runs in the
// counter direction.
`define AXI4_ASSIGN(O,I) \
  /* write address channel */\
  ``O.awaddr  = ``I.awaddr;\
  ``O.awlen   = ``I.awlen;\
  ``O.awsize  = ``I.awsize;\
  ``O.awburst = ``I.awburst;\
  ``O.awlock  = ``I.awlock;\
  ``O.awcache = ``I.awcache;\
  ``O.awprot  = ``I.awprot;\
  ``O.awqos   = ``I.awqos;\
  ``O.awvalid = ``I.awvalid;\
  ``I.awready = ``O.awready;\
  /* write data channel */\
  ``O.wdata   = ``I.wdata;\
  ``O.wstrb   = ``I.wstrb;\
  ``O.wlast   = ``I.wlast;\
  ``O.wvalid  = ``I.wvalid;\
  ``I.wready  = ``O.wready;\
  /* write resp channel */\
  ``I.bresp   = ``O.bresp;\
  ``I.bvalid  = ``O.bvalid;\
  ``O.bready  = ``I.bready;\
  /* read address channel */\
  ``O.araddr  = ``I.araddr;\
  ``O.arlen   = ``I.arlen;\
  ``O.arsize  = ``I.arsize;\
  ``O.arburst = ``I.arburst;\
  ``O.arlock  = ``I.arlock;\
  ``O.arcache = ``I.arcache;\
  ``O.arprot  = ``I.arprot;\
  ``O.arqos   = ``I.arqos;\
  ``O.arvalid = ``I.arvalid;\
  ``I.arready = ``O.arready;\
  /* read resp channel */\
  ``I.rdata   = ``O.rdata;\
  ``I.rresp   = ``O.rresp;\
  ``I.rlast   = ``O.rlast;\
  ``I.rvalid  = ``O.rvalid;\
  ``O.rready  = ``I.rready;

`define AXI4_PORT_ASSIGN(FORMAL,ACTUAL) \
  .``FORMAL``_aclk(``ACTUAL``.clk),\
  .``FORMAL``_sreset(``ACTUAL``.rst),\
  .``FORMAL``_araddr(``ACTUAL``.araddr),\
  .``FORMAL``_arlen(``ACTUAL``.arlen),\
  .``FORMAL``_arsize(``ACTUAL``.arsize),\
  .``FORMAL``_arburst(``ACTUAL``.arburst),\
  .``FORMAL``_arlock(``ACTUAL``.arlock),\
  .``FORMAL``_arcache(``ACTUAL``.arcache),\
  .``FORMAL``_arprot(``ACTUAL``.arprot),\
  .``FORMAL``_arqos(``ACTUAL``.arqos),\
  .``FORMAL``_arready(``ACTUAL``.arready),\
  .``FORMAL``_arvalid(``ACTUAL``.arvalid),\
  .``FORMAL``_awaddr(``ACTUAL``.awaddr),\
  .``FORMAL``_awlen(``ACTUAL``.awlen),\
  .``FORMAL``_awsize(``ACTUAL``.awsize),\
  .``FORMAL``_awburst(``ACTUAL``.awburst),\
  .``FORMAL``_awlock(``ACTUAL``.awlock),\
  .``FORMAL``_awcache(``ACTUAL``.awcache),\
  .``FORMAL``_awprot(``ACTUAL``.awprot),\
  .``FORMAL``_awqos(``ACTUAL``.awqos),\
  .``FORMAL``_awready(``ACTUAL``.awready),\
  .``FORMAL``_awvalid(``ACTUAL``.awvalid),\
  .``FORMAL``_bready(``ACTUAL``.bready),\
  .``FORMAL``_bresp(``ACTUAL``.bresp[1:0]),\
  .``FORMAL``_bvalid(``ACTUAL``.bvalid),\
  .``FORMAL``_rdata(``ACTUAL``.rdata),\
  .``FORMAL``_rready(``ACTUAL``.rready),\
  .``FORMAL``_rresp(``ACTUAL``.rresp[1:0]),\
  .``FORMAL``_rlast(``ACTUAL``.rlast),\
  .``FORMAL``_rvalid(``ACTUAL``.rvalid),\
  .``FORMAL``_wdata(``ACTUAL``.wdata),\
  .``FORMAL``_wready(``ACTUAL``.wready),\
  .``FORMAL``_wstrb(``ACTUAL``.wstrb),\
  .``FORMAL``_wlast(``ACTUAL``.wlast),\
  .``FORMAL``_wvalid(``ACTUAL``.wvalid),

`define AXI4_PORT_ASSIGN_NR(FORMAL,ACTUAL) \
  .``FORMAL``_araddr(``ACTUAL``.araddr),\
  .``FORMAL``_arlen(``ACTUAL``.arlen),\
  .``FORMAL``_arsize(``ACTUAL``.arsize),\
  .``FORMAL``_arburst(``ACTUAL``.arburst),\
  .``FORMAL``_arlock(``ACTUAL``.arlock),\
  .``FORMAL``_arcache(``ACTUAL``.arcache),\
  .``FORMAL``_arprot(``ACTUAL``.arprot),\
  .``FORMAL``_arqos(``ACTUAL``.arqos),\
  .``FORMAL``_arready(``ACTUAL``.arready),\
  .``FORMAL``_arvalid(``ACTUAL``.arvalid),\
  .``FORMAL``_awaddr(``ACTUAL``.awaddr),\
  .``FORMAL``_awlen(``ACTUAL``.awlen),\
  .``FORMAL``_awsize(``ACTUAL``.awsize),\
  .``FORMAL``_awburst(``ACTUAL``.awburst),\
  .``FORMAL``_awlock(``ACTUAL``.awlock),\
  .``FORMAL``_awcache(``ACTUAL``.awcache),\
  .``FORMAL``_awprot(``ACTUAL``.awprot),\
  .``FORMAL``_awqos(``ACTUAL``.awqos),\
  .``FORMAL``_awready(``ACTUAL``.awready),\
  .``FORMAL``_awvalid(``ACTUAL``.awvalid),\
  .``FORMAL``_bready(``ACTUAL``.bready),\
  .``FORMAL``_bresp(``ACTUAL``.bresp[1:0]),\
  .``FORMAL``_bvalid(``ACTUAL``.bvalid),\
  .``FORMAL``_rdata(``ACTUAL``.rdata),\
  .``FORMAL``_rready(``ACTUAL``.rready),\
  .``FORMAL``_rresp(``ACTUAL``.rresp[1:0]),\
  .``FORMAL``_rlast(``ACTUAL``.rlast),\
  .``FORMAL``_rvalid(``ACTUAL``.rvalid),\
  .``FORMAL``_wdata(``ACTUAL``.wdata),\
  .``FORMAL``_wready(``ACTUAL``.wready),\
  .``FORMAL``_wstrb(``ACTUAL``.wstrb),\
  .``FORMAL``_wlast(``ACTUAL``.wlast),\
  .``FORMAL``_wvalid(``ACTUAL``.wvalid),

`define AXI4_DEBUG_ASSIGN(O,I) \
  (* mark_debug = "true" *) logic [``I.ADDR_WIDTH-1:0]     ``I``_debug_awaddr;\
  (* mark_debug = "true" *) logic [7:0]                    ``I``_debug_awlen;\
  (* mark_debug = "true" *) logic [2:0]                    ``I``_debug_awsize;\
  (* mark_debug = "true" *) logic [1:0]                    ``I``_debug_awburst;\
  (* mark_debug = "true" *) logic [0:0]                    ``I``_debug_awlock;\
  (* mark_debug = "true" *) logic [3:0]                    ``I``_debug_awcache;\
  (* mark_debug = "true" *) logic [2:0]                    ``I``_debug_awprot;\
  (* mark_debug = "true" *) logic [3:0]                    ``I``_debug_awqos;\
  (* mark_debug = "true" *) logic                          ``I``_debug_awvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_awready;\
  (* mark_debug = "true" *) logic [``I.DATA_WIDTH-1:0]     ``I``_debug_wdata;\
  (* mark_debug = "true" *) logic [``I.BYTES_PER_WORD-1:0] ``I``_debug_wstrb;\
  (* mark_debug = "true" *) logic                          ``I``_debug_wlast;\
  (* mark_debug = "true" *) logic                          ``I``_debug_wvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_wready;\
  (* mark_debug = "true" *) logic [1:0]                    ``I``_debug_bresp;\
  (* mark_debug = "true" *) logic                          ``I``_debug_bvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_bready;\
  (* mark_debug = "true" *) logic [``I.ADDR_WIDTH-1:0]     ``I``_debug_araddr;\
  (* mark_debug = "true" *) logic [7:0]                    ``I``_debug_arlen;\
  (* mark_debug = "true" *) logic [2:0]                    ``I``_debug_arsize;\
  (* mark_debug = "true" *) logic [1:0]                    ``I``_debug_arburst;\
  (* mark_debug = "true" *) logic [0:0]                    ``I``_debug_arlock;\
  (* mark_debug = "true" *) logic [3:0]                    ``I``_debug_arcache;\
  (* mark_debug = "true" *) logic [2:0]                    ``I``_debug_arprot;\
  (* mark_debug = "true" *) logic [3:0]                    ``I``_debug_arqos;\
  (* mark_debug = "true" *) logic                          ``I``_debug_arvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_arready;\
  (* mark_debug = "true" *) logic [``I.DATA_WIDTH-1:0]     ``I``_debug_rdata;\
  (* mark_debug = "true" *) logic [1:0]                    ``I``_debug_rresp;\
  (* mark_debug = "true" *) logic                          ``I``_debug_rlast;\
  (* mark_debug = "true" *) logic                          ``I``_debug_rvalid;\
  (* mark_debug = "true" *) logic                          ``I``_debug_rready;\
  always_comb begin\
  /* write address channel */\
  ``I``_debug_awaddr  = ``I.awaddr;\
  ``I``_debug_awlen   = ``I.awlen;\
  ``I``_debug_awsize  = ``I.awsize;\
  ``I``_debug_awburst = ``I.awburst;\
  ``I``_debug_awlock  = ``I.awlock;\
  ``I``_debug_awcache = ``I.awcache;\
  ``I``_debug_awprot  = ``I.awprot;\
  ``I``_debug_awqos   = ``I.awqos;\
  ``I``_debug_awvalid = ``I.awvalid;\
  ``I.awready = ``I``_debug_awready;\
  /* write data channel */\
  ``I``_debug_wdata   = ``I.wdata;\
  ``I``_debug_wstrb   = ``I.wstrb;\
  ``I``_debug_wlast   = ``I.wlast;\
  ``I``_debug_wvalid  = ``I.wvalid;\
  ``I.wready  = ``I``_debug_wready;\
  /* write resp channel */\
  ``I.bresp   = ``I``_debug_bresp;\
  ``I.bvalid  = ``I``_debug_bvalid;\
  ``I``_debug_bready  = ``I.bready;\
  /* read address channel */\
  ``I``_debug_araddr  = ``I.araddr;\
  ``I``_debug_arlen   = ``I.arlen;\
  ``I``_debug_arsize  = ``I.arsize;\
  ``I``_debug_arburst = ``I.arburst;\
  ``I``_debug_arlock  = ``I.arlock;\
  ``I``_debug_arcache = ``I.arcache;\
  ``I``_debug_arprot  = ``I.arprot;\
  ``I``_debug_arqos   = ``I.arqos;\
  ``I``_debug_arvalid = ``I.arvalid;\
  ``I.arready = ``I``_debug_arready;\
  /* read resp channel */\
  ``I.rdata   = ``I``_debug_rdata;\
  ``I.rresp   = ``I``_debug_rresp;\
  ``I.rlast   = ``I``_debug_rlast;\
  ``I.rvalid  = ``I``_debug_rvalid;\
  ``I``_debug_rready  = ``I.rready;\
  end\
  always_comb begin\
  /* write address channel */\
  ``O.awaddr  = ``I``_debug_awaddr;\
  ``O.awlen  = ``I``_debug_awlen;\
  ``O.awsize  = ``I``_debug_awsize;\
  ``O.awburst  = ``I``_debug_awburst;\
  ``O.awlock  = ``I``_debug_awlock;\
  ``O.awcache  = ``I``_debug_awcache;\
  ``O.awprot  = ``I``_debug_awprot;\
  ``O.awqos  = ``I``_debug_awqos;\
  ``O.awvalid = ``I``_debug_awvalid;\
  ``I``_debug_awready = ``O.awready;\
  /* write data channel */\
  ``O.wdata   = ``I``_debug_wdata;\
  ``O.wstrb   = ``I``_debug_wstrb;\
  ``O.wlast   = ``I``_debug_wlast;\
  ``O.wvalid  = ``I``_debug_wvalid;\
  ``I``_debug_wready  = ``O.wready;\
  /* write resp channel */\
  ``I``_debug_bresp   = ``O.bresp;\
  ``I``_debug_bvalid  = ``O.bvalid;\
  ``O.bready  = ``I``_debug_bready;\
  /* read address channel */\
  ``O.araddr  = ``I``_debug_araddr;\
  ``O.arlen   = ``I``_debug_arlen;\
  ``O.arsize  = ``I``_debug_arsize;\
  ``O.arburst = ``I``_debug_arburst;\
  ``O.arlock  = ``I``_debug_arlock;\
  ``O.arcache = ``I``_debug_arcache;\
  ``O.arprot  = ``I``_debug_arprot;\
  ``O.arqos   = ``I``_debug_arqos;\
  ``O.arvalid = ``I``_debug_arvalid;\
  ``I``_debug_arready = ``O.arready;\
  /* read resp channel */\
  ``I``_debug_rdata   = ``O.rdata;\
  ``I``_debug_rresp   = ``O.rresp;\
  ``I``_debug_rvlast  = ``O.rlast;\
  ``I``_debug_rvalid  = ``O.rvalid;\
  ``O.rready  = ``I``_debug_rready;\
  end
