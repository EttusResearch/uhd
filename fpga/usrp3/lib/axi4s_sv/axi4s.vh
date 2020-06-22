//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Header File: axi4s
// Description: Macros for use with AXI4S
//

//-----------------------------------------------------------------------------
// Unidirectional AXI4-Stream interface
//-----------------------------------------------------------------------------

// Macro that drives o from i for all fields. Of course ready runs in the
// counter direction.

`define AXI4S_ASSIGN(O,I) \
  if (``I.TDATA) ``O.tdata = ``I.tdata;\
  if (``I.TUSER) ``O.tuser = ``I.tuser;\
  else           ``O.tuser = 0;\
  if (``I.TKEEP) ``O.tkeep = ``I.tkeep;\
  else           ``O.tkeep = '1;\
  if (``I.TLAST) ``O.tlast = ``I.tlast;\
  ``O.tvalid = ``I.tvalid;\
  ``I.tready = ``O.tready;

`define AXI4S_DEBUG_ASSIGN(O,I) \
  (* mark_debug = "true" *) logic [``I.DATA_WIDTH-1:0]   ``I``_debug_tdata;\
  (* mark_debug = "true" *) logic [``I.USER_WIDTH-1:0]   ``I``_debug_tuser;\
  (* mark_debug = "true" *) logic [``I.DATA_WIDTH/8-1:0] ``I``_debug_tkeep;\
  (* mark_debug = "true" *) logic                        ``I``_debug_tlast;\
  (* mark_debug = "true" *) logic                        ``I``_debug_tvalid;\
  (* mark_debug = "true" *) logic                        ``I``_debug_tready;\
  always_comb begin\
    if (``I.TDATA) ``I``_debug_tdata = ``I.tdata;\
    if (``I.TUSER) ``I``_debug_tuser = ``I.tuser;\
    if (``I.TKEEP) ``I``_debug_tkeep = ``I.tkeep;\
    if (``I.TLAST) ``I``_debug_tlast = ``I.tlast;\
    ``I``_debug_tvalid = ``I.tvalid;\
    ``I``_debug_tready = ``O.tready;\
  end\
  always_comb begin\
    if (``I.TDATA) ``O.tdata = ``I``_debug_tdata;\
    if (``I.TUSER) ``O.tuser = ``I``_debug_tuser;\
    else           ``O.tuser = 0;\
    if (``I.TKEEP) ``O.tkeep = ``I``_debug_tkeep;\
    else           ``O.tkeep = '1;\
    if (``I.TLAST) ``O.tlast = ``I``_debug_tlast;\
    ``O.tvalid = ``I``_debug_tvalid;\
    ``I.tready = ``I``_debug_tready;\
  end
