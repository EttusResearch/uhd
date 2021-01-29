//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Interface: AxiIf
// Description:
//  AXI4 is an ARM standard for bursting memory mapped transfers
//  For more information on the spec see
//    - https://developer.arm.com/docs/ihi0022/d
//
//  The interface contains methods for
//  (1) Writing an address
//  (2) Reading an address
//
// Parameters:
//  - DATA_WIDTH - Width of the data on AXI4 bus
//  - ADDR_WIDTH - Width of the address on AXI4 bus
//

//-----------------------------------------------------------------------------
// AXI4 interface
//-----------------------------------------------------------------------------

interface AxiIf #(
  int DATA_WIDTH = 64,
  int ADDR_WIDTH = 1
) (
  input logic clk,
  input logic rst = 1'b0
);

  import PkgAxi::*;

  localparam BYTES_PER_WORD = DATA_WIDTH/8;

  // local type defs
  typedef logic [DATA_WIDTH-1:0]     data_t;
  typedef logic [ADDR_WIDTH-1:0]     addr_t;
  typedef logic [BYTES_PER_WORD-1:0] strb_t;
  typedef logic [7:0]                len_t;
  typedef logic [2:0]                size_t;
  typedef logic [1:0]                burst_t;
  typedef logic [0:0]                lock_t;
  typedef logic [3:0]                cache_t;
  typedef logic [2:0]                prot_t;
  typedef logic [3:0]                qos_t;

  // Signals that make up an Axi interface
  // Write Address Channel
  addr_t  awaddr;
  len_t   awlen;
  size_t  awsize;
  burst_t awburst;
  lock_t  awlock;
  cache_t awcache;
  prot_t  awprot;
  qos_t   awqos;
  logic   awvalid;
  logic   awready;

  // Write Data Channel
  data_t wdata;
  strb_t wstrb = '1;
  logic  wlast;
  logic  wvalid;
  logic  wready;

  // Write Response Channel
  axi_resp_t bresp;
  logic  bvalid;
  logic  bready;

  // Read Address Channel
  addr_t  araddr;
  len_t   arlen;
  size_t  arsize;
  burst_t arburst;
  lock_t  arlock;
  cache_t arcache;
  prot_t  arprot;
  qos_t   arqos;
  logic   arvalid;
  logic   arready;

  // Read Data Channel
  data_t rdata;
  axi_resp_t rresp;
  logic  rlast;
  logic  rvalid;
  logic  rready;

  // Master Functions
  task automatic drive_aw(input addr_t  addr,
                          input len_t   len,
                          input size_t  size,
                          input burst_t burst,
                          input lock_t  lock,
                          input cache_t cache,
                          input prot_t  prot,
                          input qos_t   qos);
    awaddr  = addr;
    awlen   = len;
    awsize  = size;
    awburst = burst;
    awlock  = lock;
    awcache = cache;
    awprot  = prot;
    awqos   = qos;
    awvalid = 1;
  endtask

  task automatic drive_w(input data_t data,
                         input logic  last,
                         input strb_t strb = '1);
    wdata   = data;
    wstrb   = strb;
    wlast   = last;
    wvalid  = 1;
  endtask

  task automatic drive_aw_idle();
    awaddr  = 'X;
    awlen   = 'X;
    awsize  = 'X;
    awburst = 'X;
    awlock  = 'X;
    awcache = 'X;
    awprot  = 'X;
    awqos   = 'X;
    awvalid = 0;
  endtask

  task automatic drive_w_idle();
    wdata   = 'X;
    wstrb   = 'X;
    wlast   = 1'bX;
    wvalid  = 0;
  endtask

  task automatic drive_read(input addr_t  addr,
                            input len_t   len,
                            input size_t  size,
                            input burst_t burst,
                            input lock_t  lock,
                            input cache_t cache,
                            input prot_t  prot,
                            input qos_t   qos);
    araddr  = addr;
    arlen   = len;
    arsize  = size;
    arburst = burst;
    arlock  = lock;
    arcache = cache;
    arprot  = prot;
    arqos   = qos;
    arvalid = 1;
  endtask

  task automatic drive_read_idle();
    araddr  = 'X;
    araddr  = 'X;
    arlen   = 'X;
    arsize  = 'X;
    arburst = 'X;
    arlock  = 'X;
    arcache = 'X;
    arprot  = 'X;
    arqos   = 'X;
    arvalid = 0;
  endtask

  // Slave Functions
  task automatic drive_write_resp(input axi_resp_t resp=OKAY);
    bresp  = resp;
    bvalid = 1;
  endtask

  task automatic drive_write_resp_idle();
    bresp  = OKAY;
    bvalid = 0;
  endtask

  task automatic drive_read_resp(input data_t data,
                                 input logic  last,
                                 input axi_resp_t resp=OKAY);
    rdata  = data;
    rresp  = resp;
    rlast  = last;
    rvalid = 1;
  endtask

  task automatic drive_read_resp_idle();
    rdata  = 'X;
    rresp  = OKAY;
    rlast  = 1'bX;
    rvalid = 0;
  endtask

  // View from the master side
  modport master (
    input  clk, rst,
    output awaddr,awlen,awsize,awburst,awlock,awcache,awprot,awqos,awvalid,
           wdata,wstrb,wlast,wvalid,
           bready,
           araddr,arlen,arsize,arburst,arlock,arcache,arprot,arqos,arvalid,
           rready,
    input  awready,wready,bresp,bvalid,arready,rdata,rresp,rlast,rvalid,
    import drive_aw,
    import drive_w,
    import drive_w_idle,
    import drive_aw_idle,
    import drive_read,
    import drive_read_idle
  );

  // View from the slave side
  modport slave (
    input  clk, rst,
    input  awaddr,awlen,awsize,awburst,awlock,awcache,awprot,awqos,awvalid,
           wdata,wstrb,wlast,wvalid,
           bready,
           araddr,arlen,arsize,arburst,arlock,arcache,arprot,arqos,arvalid,
           rready,
    output awready,wready,bresp,bvalid,arready,rdata,rresp,rlast,rvalid,
    import drive_write_resp,
    import drive_write_resp_idle,
    import drive_read_resp,
    import drive_read_resp_idle
  );

endinterface : AxiIf

// The _v version of the interface does not assign any signals so it may be used
// in a continuous context.  The most common example is when associating members
// to a regular verilog output port.
interface AxiIf_v #(
  int DATA_WIDTH = 64,
  int ADDR_WIDTH = 1
) (
  input logic clk,
  input logic rst = 1'b0
);

  import PkgAxi::*;

  localparam BYTES_PER_WORD = DATA_WIDTH/8;

  // local type defs
  typedef logic [DATA_WIDTH-1:0]     data_t;
  typedef logic [ADDR_WIDTH-1:0]     addr_t;
  typedef logic [BYTES_PER_WORD-1:0] strb_t;
  typedef logic [7:0]                len_t;
  typedef logic [2:0]                size_t;
  typedef logic [1:0]                burst_t;
  typedef logic [0:0]                lock_t;
  typedef logic [3:0]                cache_t;
  typedef logic [2:0]                prot_t;
  typedef logic [3:0]                qos_t;

  // Signals that make up an Axi interface
  // Write Address Channel
  addr_t  awaddr;
  len_t   awlen;
  size_t  awsize;
  burst_t awburst;
  lock_t  awlock;
  cache_t awcache;
  prot_t  awprot;
  qos_t   awqos;
  logic   awvalid;
  logic   awready;

  // Write Data Channel
  data_t wdata;
  strb_t wstrb;
  logic  wlast;
  logic  wvalid;
  logic  wready;

  // Write Response Channel
  axi_resp_t bresp;
  logic  bvalid;
  logic  bready;

  // Read Address Channel
  addr_t  araddr;
  len_t   arlen;
  size_t  arsize;
  burst_t arburst;
  lock_t  arlock;
  cache_t arcache;
  prot_t  arprot;
  qos_t   arqos;
  logic   arvalid;
  logic   arready;

  // Read Data Channel
  data_t rdata;
  axi_resp_t rresp;
  logic  rlast;
  logic  rvalid;
  logic  rready;

  // View from the master side
  modport master (
    input  clk, rst,
    output awaddr,awlen,awsize,awburst,awlock,awcache,awprot,awqos,awvalid,
           wdata,wstrb,wlast,wvalid,
           bready,
           araddr,arlen,arsize,arburst,arlock,arcache,arprot,arqos,arvalid,
           rready,
    input  awready,wready,bresp,bvalid,arready,rdata,rresp,rlast,rvalid

  );

  // View from the slave side
  modport slave (
    input  clk, rst,
    input  awaddr,awlen,awsize,awburst,awlock,awcache,awprot,awqos,awvalid,
           wdata,wstrb,wlast,wvalid,
           bready,
           araddr,arlen,arsize,arburst,arlock,arcache,arprot,arqos,arvalid,
           rready,
    output awready,wready,bresp,bvalid,arready,rdata,rresp,rlast,rvalid

  );

endinterface : AxiIf_v
