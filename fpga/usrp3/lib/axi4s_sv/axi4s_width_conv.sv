//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//   System Verilog wrapper for axis_width_conv that accepts a AxiStreamIfc
//   with slave_user/master_user interface. This block requires that the
//   bottom bits of user contain the number of bytes in the last word.
//
//   If TKEEP - tkeep contains byte_enables
//   If USER_TRAILING_BYTES - tuser contains byte_enables
//   Else everything is a full word
//
// Parameters:
//   PIPELINE  - Add pipelining for timing.
//   SYNC_CLKS - Input and output clock are the same.
//   I_USER_TRAILING_BYTES - input byte_enable is set by user
//   O_USER_TRAILING_BYTES - output byte_enable is set in user
//

module axi4s_width_conv #(
         PIPELINE  = "NONE",
  bit    I_USER_TRAILING_BYTES = 0,
  bit    O_USER_TRAILING_BYTES = 0,
  bit    SYNC_CLKS = 1
) (
   interface.slave   i,  // AxiStreamIf or AxiStreamPacketIf
   interface.master  o   // AxiStreamIf or AxiStreamPacketIf
);

  localparam IWIDTH =i.DATA_WIDTH;
  localparam OWIDTH =o.DATA_WIDTH;

  `include "axi4s.vh"
  // Parameter Checks
  initial begin
    if(i.TKEEP) begin
      assert (!I_USER_TRAILING_BYTES) else
        $fatal(1, "I_USER_TRAILING_BYTE set at the same time as TKEEP");
      assert (!i.TUSER) else
        $fatal(1, "i.TUSER set- This module does not pass user");
    end else if(I_USER_TRAILING_BYTES) begin
      assert (i.USER_WIDTH >= i.TRAILING_WIDTH ) else
        $fatal(1, "i.USER_WIDTH does not match TRAILING_WIDTH");
    end else begin
      assert (!i.TUSER) else
        $fatal(1, "This module does not pass generic user_data");
    end

    if(o.TKEEP) begin
      assert (!O_USER_TRAILING_BYTES) else
        $fatal(1, "O_USER_TRAILING_BYTE set at the same time as TKEEP");
      assert (!o.TUSER) else
        $fatal(1, "O.TUSER set- This module does not pass user");
    end else if(O_USER_TRAILING_BYTES) begin
      assert (o.USER_WIDTH >= o.TRAILING_WIDTH) else
        $fatal(1, "o.USER_WIDTH does not match TRAILING_WIDTH");
    end else begin
      assert (!o.TUSER) else
        $fatal(1, "This module does not pass generic user_data");
    end

    assert (i.TLAST == 1) else
      $fatal(1, "i.TLAST not present");
    assert (o.TLAST == 1) else
      $fatal(1, "o.TLAST not present");
  end

  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TDATA(i.TDATA),.TKEEP(i.TKEEP),.TUSER(i.TUSER),.TLAST(i.TLAST),
    .MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s0(i.clk,i.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(o.DATA_WIDTH),.USER_WIDTH(o.USER_WIDTH),
    .TDATA(o.TDATA),.TKEEP(o.TKEEP),.TUSER(o.TUSER),.TLAST(o.TLAST),
    .MAX_PACKET_BYTES(o.MAX_PACKET_BYTES))
    s1(o.clk,o.rst);

  // move from AxiStreamIfc to AxiStreamPacketIf
  always_comb begin
    `AXI4S_ASSIGN(s0,i)
  end
  // move from AxiStreamPacketIf to AxiStreamIfc
  always_comb begin
    `AXI4S_ASSIGN(o,s1)
  end

  logic [IWIDTH/8-1:0] s0_tkeep;

  if (s0.TKEEP) begin
    always_comb s0_tkeep = s0.tkeep;
  end else if (I_USER_TRAILING_BYTES) begin
    always_comb s0_tkeep = s0.get_trailing_bytes();
  end else begin
    always_comb s0_tkeep = '1;
  end

  logic [OWIDTH/8-1:0] s1_tkeep;
  logic [15:0] s1_bytes;

  if (s1.TKEEP) begin
    always_comb s1.tkeep = s1_tkeep;
    always_comb s1.tuser = 'X;
  end else if (O_USER_TRAILING_BYTES) begin
    always_comb  s1.tkeep = 'X;
    always_comb begin : assign_s1_tuser
      s1.tuser = 0;
      // MODELSIM_BUG - deleting the s1_bytes assignment causes ModelSim failures.
      s1_bytes = s1.keep2trailing(s1_tkeep);
      s1.set_trailing_bytes(s1_tkeep);
    end
  end else begin
    always_comb s1.tkeep = 'X;
    always_comb s1.tuser = 'X;
  end

  logic s0_ready, s1_valid, s1_last;
  logic [s1.DATA_WIDTH-1:0] s1_data;
  always_comb s0.tready = s0_ready;
  always_comb s1.tvalid = s1_valid;
  always_comb s1.tlast  = s1_last;
  always_comb s1.tdata  = s1_data;

  axis_width_conv #(
    .IN_WORDS(IWIDTH/8), .OUT_WORDS(OWIDTH/8),
    .SYNC_CLKS(SYNC_CLKS), .PIPELINE(PIPELINE)
  ) axis_width_conv (
    .s_axis_aclk(s0.clk), .s_axis_rst(s0.rst),
    .s_axis_tdata(s0.tdata), .s_axis_tkeep(s0_tkeep), .s_axis_tlast(s0.tlast),
    .s_axis_tvalid(s0.tvalid), .s_axis_tready(s0_ready),

    .m_axis_aclk(s1.clk), .m_axis_rst(s1.rst),
    .m_axis_tdata(s1_data), .m_axis_tkeep(s1_tkeep), .m_axis_tlast(s1_last),
    .m_axis_tvalid(s1_valid), .m_axis_tready(s1.tready)
  );

endmodule : axi4s_width_conv
