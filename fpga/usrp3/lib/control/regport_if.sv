//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Interface: regport_if
//
// Description: RegPort interface for SystemVerilog.
//

interface regport_if #(
  int AWIDTH = 14,
  int DWIDTH = 32
) (
  input logic clk,
  input logic rst = 1'b0
);

  logic              wr_req;
  logic [AWIDTH-1:0] wr_addr;
  logic [DWIDTH-1:0] wr_data;

  logic              rd_req;
  logic [AWIDTH-1:0] rd_addr;
  logic              rd_resp;
  logic [DWIDTH-1:0] rd_data;

  // View from the master side
  modport master (
    input  clk,
    input  rst,
    output wr_req,
    output wr_addr,
    output wr_data,
    output rd_req,
    output rd_addr,
    input  rd_resp,
    input  rd_data
  );

  // View from the slave side
  modport slave (
    input  clk,
    input  rst,
    input  wr_req,
    input  wr_addr,
    input  wr_data,
    input  rd_req,
    input  rd_addr,
    output rd_resp,
    output rd_data
  );

  //---------------------------------------------------------------------------
  // Simulation Tasks
  //---------------------------------------------------------------------------

  //synthesis translate_off

  function automatic void init();
    wr_req  <= 1'b0;
    wr_addr <= 'X;
    wr_data <= 'X;
    rd_req  <= 1'b0;
    rd_addr <= 'X;
  endfunction : init

  task automatic write(bit [AWIDTH-1:0] addr, bit [DWIDTH-1:0] data);
    @(posedge clk);
    wr_req  <= 1'b1;
    wr_addr <= addr;
    wr_data <= data;
    @(posedge clk);
    init();
  endtask : write

  task automatic read(bit [AWIDTH-1:0] addr, output bit [DWIDTH-1:0] data);
    @(posedge clk);
    rd_req  <= 1'b1;
    rd_addr <= addr;
    forever begin
      @(posedge clk);
      rd_req  <= 1'b0;
      rd_addr <= 'X;
      if (rd_resp) begin
        data = rd_data;
        break;
      end
    end
    init();
  endtask : read

  //synthesis translate_on

endinterface : regport_if

