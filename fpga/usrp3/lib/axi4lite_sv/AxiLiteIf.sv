//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Interface: AxiLiteIf
// Description:
//  AXI4-LITE is an ARM standard for lighter weight registers
//  axis based on the AXI4 protocol. For more information
//  on the spec see - https://developer.arm.com/docs/ihi0022/d
//
//  The interface contains methods for
//  (1) Writing an address
//  (2) Reading an address
//
// Parameters:
//  - DATA_WIDTH - Width of the data on AXI4-Lite bus
//  - ADDR_WIDTH - Width of the address on AXI4-Lite bus
//

//-----------------------------------------------------------------------------
// AXI4-Lite interface
//-----------------------------------------------------------------------------

interface AxiLiteIf #(
  int DATA_WIDTH = 64,
  int ADDR_WIDTH = 1
) (
  input logic clk,
  input logic rst = 1'b0
);

  import PkgAxiLite::*;

  localparam BYTES_PER_WORD = DATA_WIDTH/8;

  // local type defs
  typedef logic [DATA_WIDTH-1:0]     data_t;
  typedef logic [ADDR_WIDTH-1:0]     addr_t;
  typedef logic [BYTES_PER_WORD-1:0] strb_t;

  // Signals that make up an AxiLite interface
  // Write Address Channel
  addr_t awaddr;
  logic  awvalid;
  logic  awready;

  // Write Data Channel
  data_t wdata;
  strb_t wstrb = '1;
  logic  wvalid;
  logic  wready;

  // Write Response Channel
  resp_t bresp;
  logic  bvalid;
  logic  bready;

  // Read Address Channel
  addr_t araddr;
  logic  arvalid;
  logic  arready;

  // Read Data Channel
  data_t rdata;
  resp_t rresp;
  logic  rvalid;
  logic  rready;

  // Master Functions
  task automatic drive_aw(input addr_t addr);
    awaddr  = addr;
    awvalid = 1;
  endtask

  task automatic drive_w(input data_t data,
                         input strb_t strb = '1);
    wdata   = data;
    wstrb   = strb;
    wvalid  = 1;
  endtask

  task automatic drive_aw_idle();
    awaddr  = 'X;
    awvalid = 0;
  endtask

  task automatic drive_w_idle();
    wdata   = 'X;
    wstrb   = 'X;
    wvalid  = 0;
  endtask
  task automatic drive_read(input addr_t addr);
    araddr  = addr;
    arvalid = 1;
  endtask

  task automatic drive_read_idle();
    araddr  = 'X;
    arvalid = 0;
  endtask

  // Slave Functions
  task automatic drive_write_resp(input resp_t resp=OKAY);
    bresp  = resp;
    bvalid = 1;
  endtask

  task automatic drive_write_resp_idle();
    bresp  = OKAY;
    bvalid = 0;
  endtask

  task automatic drive_read_resp(input data_t data,
                                 input resp_t resp=OKAY);
    rdata  = data;
    rresp  = resp;
    rvalid = 1;
  endtask

  task automatic drive_read_resp_idle();
    rdata  = 'X;
    rresp  = OKAY;
    rvalid = 0;
  endtask

  // Drive Functions  (These are not particularly useful
  // but they guarantee the modules using the package don't
  // drive the interface with a continuous assignment)
  task automatic drive_awaddr(input  addr_t  addr);
    awaddr = addr;
  endtask
  task automatic drive_awvalid(input  logic valid);
    awvalid = valid;
  endtask
  task automatic drive_awready(input  logic  ready);
    awready = ready;
  endtask
  task automatic drive_wdata(input  data_t data);
    wdata = data;
  endtask
  task automatic drive_wstrb(input  strb_t strb);
    wstrb = strb;
  endtask
  task automatic drive_wvalid(input  logic valid);
    wvalid = valid;
  endtask
  task automatic drive_wready(input  logic  ready);
    wready = ready;
  endtask

  task automatic drive_bresp(input  resp_t resp);
    bresp = resp;
  endtask
  task automatic drive_bvalid(input  logic valid);
    bvalid = valid;
  endtask
  task automatic drive_bready(input  logic  ready);
    bready = ready;
  endtask

  task automatic drive_araddr(input  addr_t  addr);
    araddr = addr;
  endtask
  task automatic drive_arvalid(input  logic valid);
    arvalid = valid;
  endtask
  task automatic drive_arready(input  logic  ready);
    arready = ready;
  endtask

  task automatic drive_rdata(input  data_t data);
    rdata = data;
  endtask
  task automatic drive_rresp(input  resp_t resp);
    rresp = resp;
  endtask
  task automatic drive_rvalid(input  logic valid);
    rvalid = valid;
  endtask
  task automatic drive_rready(input  logic  ready);
    rready = ready;
  endtask

  // View from the master side
  modport master (
    input  clk, rst,
    output awaddr,awvalid,wdata,wstrb,wvalid,bready,araddr,arvalid,rready,
    input  awready,wready,bresp,bvalid,arready,rdata,rresp,rvalid,
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
    input  awaddr,awvalid,wdata,wstrb,wvalid,bready,araddr,arvalid,rready,
    output awready,wready,bresp,bvalid,arready,rdata,rresp,rvalid,
    import drive_write_resp,
    import drive_write_resp_idle,
    import drive_read_resp,
    import drive_read_resp_idle
  );

endinterface : AxiLiteIf

interface AxiLiteIf_v #(
  int DATA_WIDTH = 64,
  int ADDR_WIDTH = 1
) (
  input logic clk,
  input logic rst = 1'b0
);

  import PkgAxiLite::*;

  localparam BYTES_PER_WORD = DATA_WIDTH/8;
  // local type defs
  typedef logic [DATA_WIDTH-1:0]     data_t;
  typedef logic [ADDR_WIDTH-1:0]     addr_t;
  typedef logic [BYTES_PER_WORD-1:0] strb_t;

  // Signals that make up an AxiLite interface
  // AXI-Lite
  // Write Address Channel
  addr_t awaddr;
  logic  awvalid;
  logic  awready;

  // Write Data Channel
  data_t wdata;
  strb_t wstrb;
  logic  wvalid;
  logic  wready;

  // Write Response Channel
  resp_t bresp;
  logic  bvalid;
  logic  bready;

  // Read Address Channel
  addr_t araddr;
  logic  arvalid;
  logic  arready;

  // Read Data Channel
  data_t rdata;
  resp_t rresp;
  logic  rvalid;
  logic  rready;

  // View from the master side
  modport master (
    input  clk, rst,
    output awaddr,awvalid,wdata,wstrb,wvalid,bready,araddr,arvalid,rready,
    input  awready,wready,bresp,bvalid,arready,rdata,rresp,rvalid,
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
    input  awaddr,awvalid,wdata,wstrb,wvalid,bready,araddr,arvalid,rready,
    output awready,wready,bresp,bvalid,arready,rdata,rresp,rvalid,
    import drive_write_resp,
    import drive_write_resp_idle,
    import drive_read_resp,
    import drive_read_resp_idle

  );

endinterface : AxiLiteIf_v
