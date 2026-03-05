//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: spi_slave_to_ctrlport_if_master
//
// Description:
//
//   SystemVerilog interface based wrapper of spi_slave_to_ctrlport_master.v
//   See spi_slave_to_ctrlport_master.v for more information.
//

module spi_slave_to_ctrlport_if_master
#(
  int CLK_FREQUENCY = 50_000_000,
  int SPI_FREQUENCY = 10_000_000
) (
  //---------------------------------------------------------------
  // ControlPort Master
  //---------------------------------------------------------------
  ctrlport_if.master m_ctrlport,

  //---------------------------------------------------------------
  // SPI Slave
  //---------------------------------------------------------------

  input  wire sclk,
  input  wire cs_n,
  input  wire mosi,
  output wire miso
);

  spi_slave_to_ctrlport_master #(
    .CLK_FREQUENCY (CLK_FREQUENCY),
    .SPI_FREQUENCY (SPI_FREQUENCY)
  ) spi_slave_to_ctrlport_master_i (
    .ctrlport_clk            (m_ctrlport.clk),
    .ctrlport_rst            (m_ctrlport.rst),
    .m_ctrlport_req_wr       (m_ctrlport.req.wr),
    .m_ctrlport_req_rd       (m_ctrlport.req.rd),
    .m_ctrlport_req_addr     (m_ctrlport.req.addr),
    .m_ctrlport_req_data     (m_ctrlport.req.data),
    .m_ctrlport_resp_ack     (m_ctrlport.resp.ack),
    .m_ctrlport_resp_status  (m_ctrlport.resp.status),
    .m_ctrlport_resp_data    (m_ctrlport.resp.data),
    .sclk                    (sclk),
    .cs_n                    (cs_n),
    .mosi                    (mosi),
    .miso                    (miso)
  );

endmodule
