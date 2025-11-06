//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_rfdc_info_memory
//
// Description:
//
//  This module is a bridge between the RFDC info memory and the AXI4-Lite bus.
//  The RFDC info memory contains information about the ADC / DAC configuration
//  of the RFDC block and their respective logical RFNoC channels.
//
// Parameters:
//
//  TIMEOUT          Control port will timeout after 2^TIMEOUT AXI clock cycles
//  AXI_ADDR_WIDTH   Width of the AXI bus. Aliasing occurs if AXI_ADDR_WIDTH > CTRLPORT_AWIDTH
//  CTRLPORT_AWIDTH  Number of address LSBs forwarded to m_ctrlport_req_addr
//


module axi_rfdc_info_memory #(
  parameter TIMEOUT         = 2,
  parameter AXI_ADDR_WIDTH  = 6,
  parameter CTRLPORT_AWIDTH = 6
)(
  //Clock and reset
  input  wire                  s_axi_aclk,
  input  wire                  s_axi_aresetn,
  // AXI4-Lite: Write address port (domain: s_axi_aclk)
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s AWADDR" *)
  input  wire [AXI_ADDR_WIDTH-1:0] s_axi_awaddr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s AWVALID" *)
  input  wire                  s_axi_awvalid,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s AWREADY" *)
  output wire                  s_axi_awready,
  // AXI4-Lite: Write data port (domain: s_axi_aclk)
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s WDATA" *)
  input  wire [31:0]           s_axi_wdata,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s WSTRB" *)
  input  wire [ 3:0]           s_axi_wstrb,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s WVALID" *)
  input  wire                  s_axi_wvalid,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s WREADY" *)
  output wire                  s_axi_wready,
  // AXI4-Lite: Write response port (domain: s_axi_aclk)
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s BRESP" *)
  output wire [ 1:0]           s_axi_bresp,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s BVALID" *)
  output wire                  s_axi_bvalid,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s BREADY" *)
  input  wire                  s_axi_bready,
  // AXI4-Lite: Read address port (domain: s_axi_aclk)
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s ARADDR" *)
  input  wire [AXI_ADDR_WIDTH-1:0] s_axi_araddr,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s ARVALID" *)
  input  wire                  s_axi_arvalid,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s ARREADY" *)
  output wire                  s_axi_arready,
  // AXI4-Lite: Read data port (domain: s_axi_aclk)
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s RDATA" *)
  output wire[31:0]            s_axi_rdata,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s RRESP" *)
  output wire[ 1:0]            s_axi_rresp,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s RVALID" *)
  output wire                  s_axi_rvalid,
  (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 axi_s RREADY" *)
  input  wire                  s_axi_rready
);

  // synchronize reset
  wire resetn;
  reset_sync reset_syncx (
    .clk      (s_axi_aclk),     //input wire
    .reset_in (s_axi_aresetn),  //input wire
    .reset_out(resetn)          //output reg
  );

  //ctrlport signals between memory and axi ctrlport bridge
  wire [19:0] ctrlport_req_addr;
  wire [ 3:0] ctrlport_req_byte_en;
  wire [31:0] ctrlport_req_data;
  wire        ctrlport_req_rd;
  wire        ctrlport_req_wr;
  wire        ctrlport_resp_ack;
  wire [31:0] ctrlport_resp_data;
  wire [ 1:0] ctrlport_resp_status;

  axil_ctrlport_master #(
    .TIMEOUT        (TIMEOUT),         //integer:=10
    .AXI_AWIDTH     (AXI_ADDR_WIDTH),  //integer:=17
    .CTRLPORT_AWIDTH(CTRLPORT_AWIDTH)  //integer:=17
  ) axil_ctrlport_masterx (
    .s_axi_aclk               (s_axi_aclk),            //input wire
    .s_axi_aresetn            (s_axi_aresetn),         //input wire
    .s_axi_awaddr             (s_axi_awaddr),          //input wire[(AXI_ADDR_WIDTH-1):0]
    .s_axi_awvalid            (s_axi_awvalid),         //input wire
    .s_axi_awready            (s_axi_awready),         //output reg
    .s_axi_wdata              (s_axi_wdata),           //input wire[31:0]
    .s_axi_wstrb              (s_axi_wstrb),           //input wire[3:0]
    .s_axi_wvalid             (s_axi_wvalid),          //input wire
    .s_axi_wready             (s_axi_wready),          //output reg
    .s_axi_bresp              (s_axi_bresp),           //output reg[1:0]
    .s_axi_bvalid             (s_axi_bvalid),          //output reg
    .s_axi_bready             (s_axi_bready),          //input wire
    .s_axi_araddr             (s_axi_araddr),          //input wire[(AXI_ADDR_WIDTH-1):0]
    .s_axi_arvalid            (s_axi_arvalid),         //input wire
    .s_axi_arready            (s_axi_arready),         //output reg
    .s_axi_rdata              (s_axi_rdata),           //output reg[31:0]
    .s_axi_rresp              (s_axi_rresp),           //output reg[1:0]
    .s_axi_rvalid             (s_axi_rvalid),          //output reg
    .s_axi_rready             (s_axi_rready),          //input wire
    .m_ctrlport_req_wr        (ctrlport_req_wr),       //output reg
    .m_ctrlport_req_rd        (ctrlport_req_rd),       //output reg
    .m_ctrlport_req_addr      (ctrlport_req_addr),     //output reg[19:0]
    .m_ctrlport_req_portid    (),                      //output wire[9:0]
    .m_ctrlport_req_rem_epid  (),                      //output wire[15:0]
    .m_ctrlport_req_rem_portid(),                      //output wire[9:0]
    .m_ctrlport_req_data      (ctrlport_req_data),     //output reg[31:0]
    .m_ctrlport_req_byte_en   (ctrlport_req_byte_en),  //output reg[3:0]
    .m_ctrlport_req_has_time  (),                      //output wire
    .m_ctrlport_req_time      (),                      //output wire[63:0]
    .m_ctrlport_resp_ack      (ctrlport_resp_ack),     //input wire
    .m_ctrlport_resp_status   (ctrlport_resp_status),  //input wire[1:0]
    .m_ctrlport_resp_data     (ctrlport_resp_data)     //input wire[31:0]
  );

  rfdc_info_memory rfdc_info_memoryx (
    .clk                   (s_axi_aclk),            //input logic
    .rst                   (~resetn),               //input logic
    .s_ctrlport_req_addr   (ctrlport_req_addr),     //input logic[19:0]
    .s_ctrlport_req_byte_en(ctrlport_req_byte_en),  //input logic[3:0]
    .s_ctrlport_req_data   (ctrlport_req_data),     //input logic[31:0]
    .s_ctrlport_req_rd     (ctrlport_req_rd),       //input logic
    .s_ctrlport_req_wr     (ctrlport_req_wr),       //input logic
    .s_ctrlport_resp_ack   (ctrlport_resp_ack),     //output logic
    .s_ctrlport_resp_data  (ctrlport_resp_data),    //output logic[31:0]
    .s_ctrlport_resp_status(ctrlport_resp_status)   //output logic[1:0]
  );

endmodule
