//Copyright 1986-2015 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2015.4 (lin64) Build 1412921 Wed Nov 18 09:44:32 MST 2015
//Date        : Mon Oct 24 19:58:35 2016
//Host        : ubuntu-VM running 64-bit Ubuntu 14.04.5 LTS
//Command     : generate_target axi_intercon_2x64_128_bd_wrapper.bd
//Design      : axi_intercon_2x64_128_bd_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module axi_intercon_2x64_128_bd_wrapper
   (M00_AXI_ACLK,
    M00_AXI_ARESETN,
    M00_AXI_ARADDR,
    M00_AXI_ARBURST,
    M00_AXI_ARCACHE,
    M00_AXI_ARID,
    M00_AXI_ARLEN,
    M00_AXI_ARLOCK,
    M00_AXI_ARPROT,
    M00_AXI_ARQOS,
    M00_AXI_ARREADY,
    M00_AXI_ARREGION,
    M00_AXI_ARSIZE,
    M00_AXI_ARVALID,
    M00_AXI_AWADDR,
    M00_AXI_AWBURST,
    M00_AXI_AWCACHE,
    M00_AXI_AWID,
    M00_AXI_AWLEN,
    M00_AXI_AWLOCK,
    M00_AXI_AWPROT,
    M00_AXI_AWQOS,
    M00_AXI_AWREADY,
    M00_AXI_AWREGION,
    M00_AXI_AWSIZE,
    M00_AXI_AWVALID,
    M00_AXI_BID,
    M00_AXI_BREADY,
    M00_AXI_BRESP,
    M00_AXI_BVALID,
    M00_AXI_RDATA,
    M00_AXI_RID,
    M00_AXI_RLAST,
    M00_AXI_RREADY,
    M00_AXI_RRESP,
    M00_AXI_RVALID,
    M00_AXI_WDATA,
    M00_AXI_WLAST,
    M00_AXI_WREADY,
    M00_AXI_WSTRB,
    M00_AXI_WVALID,
    S00_AXI_ACLK,
    S00_AXI_ARESETN,
    S00_AXI_ARADDR,
    S00_AXI_ARBURST,
    S00_AXI_ARCACHE,
    S00_AXI_ARID,
    S00_AXI_ARLEN,
    S00_AXI_ARLOCK,
    S00_AXI_ARPROT,
    S00_AXI_ARQOS,
    S00_AXI_ARREADY,
    S00_AXI_ARREGION,
    S00_AXI_ARSIZE,
    S00_AXI_ARVALID,
    S00_AXI_AWADDR,
    S00_AXI_AWBURST,
    S00_AXI_AWCACHE,
    S00_AXI_AWID,
    S00_AXI_AWLEN,
    S00_AXI_AWLOCK,
    S00_AXI_AWPROT,
    S00_AXI_AWQOS,
    S00_AXI_AWREADY,
    S00_AXI_AWREGION,
    S00_AXI_AWSIZE,
    S00_AXI_AWVALID,
    S00_AXI_BID,
    S00_AXI_BREADY,
    S00_AXI_BRESP,
    S00_AXI_BVALID,
    S00_AXI_RDATA,
    S00_AXI_RID,
    S00_AXI_RLAST,
    S00_AXI_RREADY,
    S00_AXI_RRESP,
    S00_AXI_RVALID,
    S00_AXI_WDATA,
    S00_AXI_WLAST,
    S00_AXI_WREADY,
    S00_AXI_WSTRB,
    S00_AXI_WVALID,
    S01_AXI_ACLK,
    S01_AXI_ARESETN,
    S01_AXI_ARADDR,
    S01_AXI_ARBURST,
    S01_AXI_ARCACHE,
    S01_AXI_ARID,
    S01_AXI_ARLEN,
    S01_AXI_ARLOCK,
    S01_AXI_ARPROT,
    S01_AXI_ARQOS,
    S01_AXI_ARREADY,
    S01_AXI_ARREGION,
    S01_AXI_ARSIZE,
    S01_AXI_ARVALID,
    S01_AXI_AWADDR,
    S01_AXI_AWBURST,
    S01_AXI_AWCACHE,
    S01_AXI_AWID,
    S01_AXI_AWLEN,
    S01_AXI_AWLOCK,
    S01_AXI_AWPROT,
    S01_AXI_AWQOS,
    S01_AXI_AWREADY,
    S01_AXI_AWREGION,
    S01_AXI_AWSIZE,
    S01_AXI_AWVALID,
    S01_AXI_BID,
    S01_AXI_BREADY,
    S01_AXI_BRESP,
    S01_AXI_BVALID,
    S01_AXI_RDATA,
    S01_AXI_RID,
    S01_AXI_RLAST,
    S01_AXI_RREADY,
    S01_AXI_RRESP,
    S01_AXI_RVALID,
    S01_AXI_WDATA,
    S01_AXI_WLAST,
    S01_AXI_WREADY,
    S01_AXI_WSTRB,
    S01_AXI_WVALID);
  input M00_AXI_ACLK;
  input M00_AXI_ARESETN;
  output [31:0]M00_AXI_ARADDR;
  output [1:0]M00_AXI_ARBURST;
  output [3:0]M00_AXI_ARCACHE;
  output [0:0]M00_AXI_ARID;
  output [7:0]M00_AXI_ARLEN;
  output [0:0]M00_AXI_ARLOCK;
  output [2:0]M00_AXI_ARPROT;
  output [3:0]M00_AXI_ARQOS;
  input M00_AXI_ARREADY;
  output [3:0]M00_AXI_ARREGION;
  output [2:0]M00_AXI_ARSIZE;
  output M00_AXI_ARVALID;
  output [31:0]M00_AXI_AWADDR;
  output [1:0]M00_AXI_AWBURST;
  output [3:0]M00_AXI_AWCACHE;
  output [0:0]M00_AXI_AWID;
  output [7:0]M00_AXI_AWLEN;
  output [0:0]M00_AXI_AWLOCK;
  output [2:0]M00_AXI_AWPROT;
  output [3:0]M00_AXI_AWQOS;
  input M00_AXI_AWREADY;
  output [3:0]M00_AXI_AWREGION;
  output [2:0]M00_AXI_AWSIZE;
  output M00_AXI_AWVALID;
  input [0:0]M00_AXI_BID;
  output M00_AXI_BREADY;
  input [1:0]M00_AXI_BRESP;
  input M00_AXI_BVALID;
  input [255:0]M00_AXI_RDATA;
  input [0:0]M00_AXI_RID;
  input M00_AXI_RLAST;
  output M00_AXI_RREADY;
  input [1:0]M00_AXI_RRESP;
  input M00_AXI_RVALID;
  output [255:0]M00_AXI_WDATA;
  output M00_AXI_WLAST;
  input M00_AXI_WREADY;
  output [31:0]M00_AXI_WSTRB;
  output M00_AXI_WVALID;
  input S00_AXI_ACLK;
  input S00_AXI_ARESETN;
  input [31:0]S00_AXI_ARADDR;
  input [1:0]S00_AXI_ARBURST;
  input [3:0]S00_AXI_ARCACHE;
  input [0:0]S00_AXI_ARID;
  input [7:0]S00_AXI_ARLEN;
  input [0:0]S00_AXI_ARLOCK;
  input [2:0]S00_AXI_ARPROT;
  input [3:0]S00_AXI_ARQOS;
  output S00_AXI_ARREADY;
  input [3:0]S00_AXI_ARREGION;
  input [2:0]S00_AXI_ARSIZE;
  input S00_AXI_ARVALID;
  input [31:0]S00_AXI_AWADDR;
  input [1:0]S00_AXI_AWBURST;
  input [3:0]S00_AXI_AWCACHE;
  input [0:0]S00_AXI_AWID;
  input [7:0]S00_AXI_AWLEN;
  input [0:0]S00_AXI_AWLOCK;
  input [2:0]S00_AXI_AWPROT;
  input [3:0]S00_AXI_AWQOS;
  output S00_AXI_AWREADY;
  input [3:0]S00_AXI_AWREGION;
  input [2:0]S00_AXI_AWSIZE;
  input S00_AXI_AWVALID;
  output [0:0]S00_AXI_BID;
  input S00_AXI_BREADY;
  output [1:0]S00_AXI_BRESP;
  output S00_AXI_BVALID;
  output [63:0]S00_AXI_RDATA;
  output [0:0]S00_AXI_RID;
  output S00_AXI_RLAST;
  input S00_AXI_RREADY;
  output [1:0]S00_AXI_RRESP;
  output S00_AXI_RVALID;
  input [63:0]S00_AXI_WDATA;
  input S00_AXI_WLAST;
  output S00_AXI_WREADY;
  input [7:0]S00_AXI_WSTRB;
  input S00_AXI_WVALID;
  input S01_AXI_ACLK;
  input S01_AXI_ARESETN;
  input [31:0]S01_AXI_ARADDR;
  input [1:0]S01_AXI_ARBURST;
  input [3:0]S01_AXI_ARCACHE;
  input [0:0]S01_AXI_ARID;
  input [7:0]S01_AXI_ARLEN;
  input [0:0]S01_AXI_ARLOCK;
  input [2:0]S01_AXI_ARPROT;
  input [3:0]S01_AXI_ARQOS;
  output S01_AXI_ARREADY;
  input [3:0]S01_AXI_ARREGION;
  input [2:0]S01_AXI_ARSIZE;
  input S01_AXI_ARVALID;
  input [31:0]S01_AXI_AWADDR;
  input [1:0]S01_AXI_AWBURST;
  input [3:0]S01_AXI_AWCACHE;
  input [0:0]S01_AXI_AWID;
  input [7:0]S01_AXI_AWLEN;
  input [0:0]S01_AXI_AWLOCK;
  input [2:0]S01_AXI_AWPROT;
  input [3:0]S01_AXI_AWQOS;
  output S01_AXI_AWREADY;
  input [3:0]S01_AXI_AWREGION;
  input [2:0]S01_AXI_AWSIZE;
  input S01_AXI_AWVALID;
  output [0:0]S01_AXI_BID;
  input S01_AXI_BREADY;
  output [1:0]S01_AXI_BRESP;
  output S01_AXI_BVALID;
  output [63:0]S01_AXI_RDATA;
  output [0:0]S01_AXI_RID;
  output S01_AXI_RLAST;
  input S01_AXI_RREADY;
  output [1:0]S01_AXI_RRESP;
  output S01_AXI_RVALID;
  input [63:0]S01_AXI_WDATA;
  input S01_AXI_WLAST;
  output S01_AXI_WREADY;
  input [7:0]S01_AXI_WSTRB;
  input S01_AXI_WVALID;

  wire M00_AXI_ACLK;
  wire M00_AXI_ARESETN;
  wire [31:0]M00_AXI_ARADDR;
  wire [1:0]M00_AXI_ARBURST;
  wire [3:0]M00_AXI_ARCACHE;
  wire [0:0]M00_AXI_ARID;
  wire [7:0]M00_AXI_ARLEN;
  wire [0:0]M00_AXI_ARLOCK;
  wire [2:0]M00_AXI_ARPROT;
  wire [3:0]M00_AXI_ARQOS;
  wire M00_AXI_ARREADY;
  wire [3:0]M00_AXI_ARREGION;
  wire [2:0]M00_AXI_ARSIZE;
  wire M00_AXI_ARVALID;
  wire [31:0]M00_AXI_AWADDR;
  wire [1:0]M00_AXI_AWBURST;
  wire [3:0]M00_AXI_AWCACHE;
  wire [0:0]M00_AXI_AWID;
  wire [7:0]M00_AXI_AWLEN;
  wire [0:0]M00_AXI_AWLOCK;
  wire [2:0]M00_AXI_AWPROT;
  wire [3:0]M00_AXI_AWQOS;
  wire M00_AXI_AWREADY;
  wire [3:0]M00_AXI_AWREGION;
  wire [2:0]M00_AXI_AWSIZE;
  wire M00_AXI_AWVALID;
  wire [0:0]M00_AXI_BID;
  wire M00_AXI_BREADY;
  wire [1:0]M00_AXI_BRESP;
  wire M00_AXI_BVALID;
  wire [255:0]M00_AXI_RDATA;
  wire [0:0]M00_AXI_RID;
  wire M00_AXI_RLAST;
  wire M00_AXI_RREADY;
  wire [1:0]M00_AXI_RRESP;
  wire M00_AXI_RVALID;
  wire [255:0]M00_AXI_WDATA;
  wire M00_AXI_WLAST;
  wire M00_AXI_WREADY;
  wire [31:0]M00_AXI_WSTRB;
  wire M00_AXI_WVALID;
  wire S00_AXI_ACLK;
  wire S00_AXI_ARESETN;
  wire [31:0]S00_AXI_ARADDR;
  wire [1:0]S00_AXI_ARBURST;
  wire [3:0]S00_AXI_ARCACHE;
  wire [0:0]S00_AXI_ARID;
  wire [7:0]S00_AXI_ARLEN;
  wire [0:0]S00_AXI_ARLOCK;
  wire [2:0]S00_AXI_ARPROT;
  wire [3:0]S00_AXI_ARQOS;
  wire S00_AXI_ARREADY;
  wire [3:0]S00_AXI_ARREGION;
  wire [2:0]S00_AXI_ARSIZE;
  wire S00_AXI_ARVALID;
  wire [31:0]S00_AXI_AWADDR;
  wire [1:0]S00_AXI_AWBURST;
  wire [3:0]S00_AXI_AWCACHE;
  wire [0:0]S00_AXI_AWID;
  wire [7:0]S00_AXI_AWLEN;
  wire [0:0]S00_AXI_AWLOCK;
  wire [2:0]S00_AXI_AWPROT;
  wire [3:0]S00_AXI_AWQOS;
  wire S00_AXI_AWREADY;
  wire [3:0]S00_AXI_AWREGION;
  wire [2:0]S00_AXI_AWSIZE;
  wire S00_AXI_AWVALID;
  wire [0:0]S00_AXI_BID;
  wire S00_AXI_BREADY;
  wire [1:0]S00_AXI_BRESP;
  wire S00_AXI_BVALID;
  wire [63:0]S00_AXI_RDATA;
  wire [0:0]S00_AXI_RID;
  wire S00_AXI_RLAST;
  wire S00_AXI_RREADY;
  wire [1:0]S00_AXI_RRESP;
  wire S00_AXI_RVALID;
  wire [63:0]S00_AXI_WDATA;
  wire S00_AXI_WLAST;
  wire S00_AXI_WREADY;
  wire [7:0]S00_AXI_WSTRB;
  wire S00_AXI_WVALID;
  wire S01_AXI_ACLK;
  wire S01_AXI_ARESETN;
  wire [31:0]S01_AXI_ARADDR;
  wire [1:0]S01_AXI_ARBURST;
  wire [3:0]S01_AXI_ARCACHE;
  wire [0:0]S01_AXI_ARID;
  wire [7:0]S01_AXI_ARLEN;
  wire [0:0]S01_AXI_ARLOCK;
  wire [2:0]S01_AXI_ARPROT;
  wire [3:0]S01_AXI_ARQOS;
  wire S01_AXI_ARREADY;
  wire [3:0]S01_AXI_ARREGION;
  wire [2:0]S01_AXI_ARSIZE;
  wire S01_AXI_ARVALID;
  wire [31:0]S01_AXI_AWADDR;
  wire [1:0]S01_AXI_AWBURST;
  wire [3:0]S01_AXI_AWCACHE;
  wire [0:0]S01_AXI_AWID;
  wire [7:0]S01_AXI_AWLEN;
  wire [0:0]S01_AXI_AWLOCK;
  wire [2:0]S01_AXI_AWPROT;
  wire [3:0]S01_AXI_AWQOS;
  wire S01_AXI_AWREADY;
  wire [3:0]S01_AXI_AWREGION;
  wire [2:0]S01_AXI_AWSIZE;
  wire S01_AXI_AWVALID;
  wire [0:0]S01_AXI_BID;
  wire S01_AXI_BREADY;
  wire [1:0]S01_AXI_BRESP;
  wire S01_AXI_BVALID;
  wire [63:0]S01_AXI_RDATA;
  wire [0:0]S01_AXI_RID;
  wire S01_AXI_RLAST;
  wire S01_AXI_RREADY;
  wire [1:0]S01_AXI_RRESP;
  wire S01_AXI_RVALID;
  wire [63:0]S01_AXI_WDATA;
  wire S01_AXI_WLAST;
  wire S01_AXI_WREADY;
  wire [7:0]S01_AXI_WSTRB;
  wire S01_AXI_WVALID;

  axi_intercon_2x64_128_bd axi_intercon_2x64_128_bd_i
       (.M00_AXI_ACLK(M00_AXI_ACLK),
        .M00_AXI_ARESETN(M00_AXI_ARESETN),
        .M00_AXI_araddr(M00_AXI_ARADDR),
        .M00_AXI_arburst(M00_AXI_ARBURST),
        .M00_AXI_arcache(M00_AXI_ARCACHE),
        .M00_AXI_arid(M00_AXI_ARID),
        .M00_AXI_arlen(M00_AXI_ARLEN),
        .M00_AXI_arlock(M00_AXI_ARLOCK),
        .M00_AXI_arprot(M00_AXI_ARPROT),
        .M00_AXI_arqos(M00_AXI_ARQOS),
        .M00_AXI_arready(M00_AXI_ARREADY),
        .M00_AXI_arregion(M00_AXI_ARREGION),
        .M00_AXI_arsize(M00_AXI_ARSIZE),
        .M00_AXI_arvalid(M00_AXI_ARVALID),
        .M00_AXI_awaddr(M00_AXI_AWADDR),
        .M00_AXI_awburst(M00_AXI_AWBURST),
        .M00_AXI_awcache(M00_AXI_AWCACHE),
        .M00_AXI_awid(M00_AXI_AWID),
        .M00_AXI_awlen(M00_AXI_AWLEN),
        .M00_AXI_awlock(M00_AXI_AWLOCK),
        .M00_AXI_awprot(M00_AXI_AWPROT),
        .M00_AXI_awqos(M00_AXI_AWQOS),
        .M00_AXI_awready(M00_AXI_AWREADY),
        .M00_AXI_awregion(M00_AXI_AWREGION),
        .M00_AXI_awsize(M00_AXI_AWSIZE),
        .M00_AXI_awvalid(M00_AXI_AWVALID),
        .M00_AXI_bid(M00_AXI_BID),
        .M00_AXI_bready(M00_AXI_BREADY),
        .M00_AXI_bresp(M00_AXI_BRESP),
        .M00_AXI_bvalid(M00_AXI_BVALID),
        .M00_AXI_rdata(M00_AXI_RDATA),
        .M00_AXI_rid(M00_AXI_RID),
        .M00_AXI_rlast(M00_AXI_RLAST),
        .M00_AXI_rready(M00_AXI_RREADY),
        .M00_AXI_rresp(M00_AXI_RRESP),
        .M00_AXI_rvalid(M00_AXI_RVALID),
        .M00_AXI_wdata(M00_AXI_WDATA),
        .M00_AXI_wlast(M00_AXI_WLAST),
        .M00_AXI_wready(M00_AXI_WREADY),
        .M00_AXI_wstrb(M00_AXI_WSTRB),
        .M00_AXI_wvalid(M00_AXI_WVALID),
        .S00_AXI_ACLK(S00_AXI_ACLK),
        .S00_AXI_ARESETN(S00_AXI_ARESETN),
        .S00_AXI_araddr(S00_AXI_ARADDR),
        .S00_AXI_arburst(S00_AXI_ARBURST),
        .S00_AXI_arcache(S00_AXI_ARCACHE),
        .S00_AXI_arid(S00_AXI_ARID),
        .S00_AXI_arlen(S00_AXI_ARLEN),
        .S00_AXI_arlock(S00_AXI_ARLOCK),
        .S00_AXI_arprot(S00_AXI_ARPROT),
        .S00_AXI_arqos(S00_AXI_ARQOS),
        .S00_AXI_arready(S00_AXI_ARREADY),
        .S00_AXI_arregion(S00_AXI_ARREGION),
        .S00_AXI_arsize(S00_AXI_ARSIZE),
        .S00_AXI_arvalid(S00_AXI_ARVALID),
        .S00_AXI_awaddr(S00_AXI_AWADDR),
        .S00_AXI_awburst(S00_AXI_AWBURST),
        .S00_AXI_awcache(S00_AXI_AWCACHE),
        .S00_AXI_awid(S00_AXI_AWID),
        .S00_AXI_awlen(S00_AXI_AWLEN),
        .S00_AXI_awlock(S00_AXI_AWLOCK),
        .S00_AXI_awprot(S00_AXI_AWPROT),
        .S00_AXI_awqos(S00_AXI_AWQOS),
        .S00_AXI_awready(S00_AXI_AWREADY),
        .S00_AXI_awregion(S00_AXI_AWREGION),
        .S00_AXI_awsize(S00_AXI_AWSIZE),
        .S00_AXI_awvalid(S00_AXI_AWVALID),
        .S00_AXI_bid(S00_AXI_BID),
        .S00_AXI_bready(S00_AXI_BREADY),
        .S00_AXI_bresp(S00_AXI_BRESP),
        .S00_AXI_bvalid(S00_AXI_BVALID),
        .S00_AXI_rdata(S00_AXI_RDATA),
        .S00_AXI_rid(S00_AXI_RID),
        .S00_AXI_rlast(S00_AXI_RLAST),
        .S00_AXI_rready(S00_AXI_RREADY),
        .S00_AXI_rresp(S00_AXI_RRESP),
        .S00_AXI_rvalid(S00_AXI_RVALID),
        .S00_AXI_wdata(S00_AXI_WDATA),
        .S00_AXI_wlast(S00_AXI_WLAST),
        .S00_AXI_wready(S00_AXI_WREADY),
        .S00_AXI_wstrb(S00_AXI_WSTRB),
        .S00_AXI_wvalid(S00_AXI_WVALID),
        .S01_AXI_ACLK(S01_AXI_ACLK),
        .S01_AXI_ARESETN(S01_AXI_ARESETN),
        .S01_AXI_araddr(S01_AXI_ARADDR),
        .S01_AXI_arburst(S01_AXI_ARBURST),
        .S01_AXI_arcache(S01_AXI_ARCACHE),
        .S01_AXI_arid(S01_AXI_ARID),
        .S01_AXI_arlen(S01_AXI_ARLEN),
        .S01_AXI_arlock(S01_AXI_ARLOCK),
        .S01_AXI_arprot(S01_AXI_ARPROT),
        .S01_AXI_arqos(S01_AXI_ARQOS),
        .S01_AXI_arready(S01_AXI_ARREADY),
        .S01_AXI_arregion(S01_AXI_ARREGION),
        .S01_AXI_arsize(S01_AXI_ARSIZE),
        .S01_AXI_arvalid(S01_AXI_ARVALID),
        .S01_AXI_awaddr(S01_AXI_AWADDR),
        .S01_AXI_awburst(S01_AXI_AWBURST),
        .S01_AXI_awcache(S01_AXI_AWCACHE),
        .S01_AXI_awid(S01_AXI_AWID),
        .S01_AXI_awlen(S01_AXI_AWLEN),
        .S01_AXI_awlock(S01_AXI_AWLOCK),
        .S01_AXI_awprot(S01_AXI_AWPROT),
        .S01_AXI_awqos(S01_AXI_AWQOS),
        .S01_AXI_awready(S01_AXI_AWREADY),
        .S01_AXI_awregion(S01_AXI_AWREGION),
        .S01_AXI_awsize(S01_AXI_AWSIZE),
        .S01_AXI_awvalid(S01_AXI_AWVALID),
        .S01_AXI_bid(S01_AXI_BID),
        .S01_AXI_bready(S01_AXI_BREADY),
        .S01_AXI_bresp(S01_AXI_BRESP),
        .S01_AXI_bvalid(S01_AXI_BVALID),
        .S01_AXI_rdata(S01_AXI_RDATA),
        .S01_AXI_rid(S01_AXI_RID),
        .S01_AXI_rlast(S01_AXI_RLAST),
        .S01_AXI_rready(S01_AXI_RREADY),
        .S01_AXI_rresp(S01_AXI_RRESP),
        .S01_AXI_rvalid(S01_AXI_RVALID),
        .S01_AXI_wdata(S01_AXI_WDATA),
        .S01_AXI_wlast(S01_AXI_WLAST),
        .S01_AXI_wready(S01_AXI_WREADY),
        .S01_AXI_wstrb(S01_AXI_WSTRB),
        .S01_AXI_wvalid(S01_AXI_WVALID));
endmodule
