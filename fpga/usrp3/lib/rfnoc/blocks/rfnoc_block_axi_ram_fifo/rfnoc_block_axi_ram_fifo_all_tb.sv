//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_axi_ram_fifo_all_tb
//
// Description:
//
//   This is the testbench for rfnoc_block_axi_ram_fifo that instantiates 
//   several variations of rfnoc_block_axi_ram_fifo_tb to test different 
//   configurations.
//


module rfnoc_block_axi_ram_fifo_all_tb;

  timeunit 1ns;
  timeprecision 1ps;

  import PkgTestExec::*;


  //---------------------------------------------------------------------------
  // Test Definitions
  //---------------------------------------------------------------------------

  typedef struct {
    int CHDR_W;
    int NUM_PORTS;
    int MEM_DATA_W;
    int MEM_ADDR_W;
    int FIFO_ADDR_W;
    int IN_FIFO_SIZE;
    int OUT_FIFO_SIZE;
    bit OVERFLOW;
    bit BIST;
  } test_config_t;

  localparam NUM_TESTS = 4;

  localparam test_config_t test[NUM_TESTS] = '{
    '{CHDR_W:  64, NUM_PORTS: 2, MEM_DATA_W:  64, MEM_ADDR_W: 13, FIFO_ADDR_W: 12, IN_FIFO_SIZE:  9, OUT_FIFO_SIZE:  9, OVERFLOW: 1, BIST: 1 },
    '{CHDR_W:  64, NUM_PORTS: 1, MEM_DATA_W: 128, MEM_ADDR_W: 14, FIFO_ADDR_W: 13, IN_FIFO_SIZE:  9, OUT_FIFO_SIZE:  9, OVERFLOW: 1, BIST: 1 },
    '{CHDR_W: 128, NUM_PORTS: 1, MEM_DATA_W:  64, MEM_ADDR_W: 13, FIFO_ADDR_W: 12, IN_FIFO_SIZE:  9, OUT_FIFO_SIZE: 10, OVERFLOW: 0, BIST: 1 },
    '{CHDR_W: 128, NUM_PORTS: 1, MEM_DATA_W: 128, MEM_ADDR_W: 16, FIFO_ADDR_W: 14, IN_FIFO_SIZE: 12, OUT_FIFO_SIZE: 12, OVERFLOW: 0, BIST: 0 }
  };


  //---------------------------------------------------------------------------
  // DUT Instances
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_TESTS; i++) begin : gen_test_config
    rfnoc_block_axi_ram_fifo_tb #(
      .CHDR_W        (test[i].CHDR_W),
      .NUM_PORTS     (test[i].NUM_PORTS),
      .MEM_DATA_W    (test[i].MEM_DATA_W),
      .MEM_ADDR_W    (test[i].MEM_ADDR_W),
      .FIFO_ADDR_W   (test[i].FIFO_ADDR_W),
      .IN_FIFO_SIZE  (test[i].IN_FIFO_SIZE),
      .OUT_FIFO_SIZE (test[i].OUT_FIFO_SIZE),
      .OVERFLOW      (test[i].OVERFLOW),
      .BIST          (test[i].BIST)
    ) rfnoc_block_radio_tb_i ();
  end : gen_test_config


endmodule : rfnoc_block_axi_ram_fifo_all_tb