//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_byte_deserializer
//
// Description:
//
//   SystemVerilog interface based wrapper of ctrlport_byte_deserializer.v
//   See ctrlport_byte_deserializer.v for more information.

module ctrlport_if_byte_deserializer (
  //---------------------------------------------------------------
  // ControlPort Master
  //---------------------------------------------------------------
  ctrlport_if.master m_ctrlport,

  //---------------------------------------------------------------
  // Byte Interface
  //---------------------------------------------------------------
  // clock domain used to register input data is shared with ctrlport interface
  // output clock used to drive output data
  input  logic output_clk,

  // byte interface
  input  logic [7:0] bytestream_data_in,
  input  logic       bytestream_valid_in,
  input  logic       bytestream_direction,
  output logic [7:0] bytestream_data_out,
  output logic       bytestream_valid_out,
  output logic       bytestream_output_enable
);

  ctrlport_byte_deserializer ctrlport_byte_deserializer_i (
    .ctrlport_clk              (m_ctrlport.clk),
    .ctrlport_clk_adjusted     (output_clk),
    .ctrlport_rst              (m_ctrlport.rst),
    .m_ctrlport_req_wr         (m_ctrlport.req.wr),
    .m_ctrlport_req_rd         (m_ctrlport.req.rd),
    .m_ctrlport_req_addr       (m_ctrlport.req.addr),
    .m_ctrlport_req_data       (m_ctrlport.req.data),
    .m_ctrlport_resp_ack       (m_ctrlport.resp.ack),
    .m_ctrlport_resp_status    (m_ctrlport.resp.status),
    .m_ctrlport_resp_data      (m_ctrlport.resp.data),
    .bytestream_data_in        (bytestream_data_in),
    .bytestream_valid_in       (bytestream_valid_in),
    .bytestream_direction      (bytestream_direction),
    .bytestream_data_out       (bytestream_data_out),
    .bytestream_valid_out      (bytestream_valid_out),
    .bytestream_output_enable  (bytestream_output_enable)
  );

endmodule
