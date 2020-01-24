//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_radio_all_tb
//
// Description: This is the testbench for rfnoc_block_radio that instantiates 
// several variations of rfnoc_block_radio_tb to test different configurations.
//


module rfnoc_block_radio_all_tb;

  timeunit 1ns;
  timeprecision 1ps;

  import PkgTestExec::*;


  //---------------------------------------------------------------------------
  // Test Definitions
  //---------------------------------------------------------------------------

  typedef struct {
    int CHDR_W;
    int ITEM_W;
    int NIPC;
    int NUM_PORTS;
    int STALL_PROB;
    int STB_PROB;
    bit TEST_REGS;
  } test_config_t;

  localparam NUM_TESTS = 9;

  localparam test_config_t test[NUM_TESTS] = '{
    '{CHDR_W:  64, ITEM_W: 16, NIPC: 1, NUM_PORTS: 3, STALL_PROB: 10, STB_PROB: 100, TEST_REGS: 1 },
    '{CHDR_W:  64, ITEM_W: 16, NIPC: 1, NUM_PORTS: 2, STALL_PROB: 25, STB_PROB:  80, TEST_REGS: 1 },
    '{CHDR_W:  64, ITEM_W: 16, NIPC: 2, NUM_PORTS: 1, STALL_PROB: 25, STB_PROB:  80, TEST_REGS: 0 },
    '{CHDR_W:  64, ITEM_W: 32, NIPC: 1, NUM_PORTS: 1, STALL_PROB: 25, STB_PROB:  80, TEST_REGS: 0 },
    '{CHDR_W:  64, ITEM_W: 32, NIPC: 2, NUM_PORTS: 1, STALL_PROB: 10, STB_PROB:  80, TEST_REGS: 0 },
    '{CHDR_W: 128, ITEM_W: 32, NIPC: 1, NUM_PORTS: 3, STALL_PROB: 10, STB_PROB: 100, TEST_REGS: 1 },
    '{CHDR_W: 128, ITEM_W: 32, NIPC: 1, NUM_PORTS: 2, STALL_PROB: 25, STB_PROB:  80, TEST_REGS: 0 },
    '{CHDR_W: 128, ITEM_W: 32, NIPC: 2, NUM_PORTS: 1, STALL_PROB: 25, STB_PROB:  80, TEST_REGS: 0 },
    '{CHDR_W: 128, ITEM_W: 32, NIPC: 4, NUM_PORTS: 1, STALL_PROB: 10, STB_PROB:  80, TEST_REGS: 0 }
  };


  //---------------------------------------------------------------------------
  // DUT Instances
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_TESTS; i++) begin : gen_test_config
    rfnoc_block_radio_tb #(
      .CHDR_W     (test[i].CHDR_W    ),
      .ITEM_W     (test[i].ITEM_W    ),
      .NIPC       (test[i].NIPC      ),
      .NUM_PORTS  (test[i].NUM_PORTS ),
      .STALL_PROB (test[i].STALL_PROB),
      .STB_PROB   (test[i].STB_PROB  ),
      .TEST_REGS  (test[i].TEST_REGS )
    ) rfnoc_block_radio_tb_i ();
  end : gen_test_config


endmodule : rfnoc_block_radio_all_tb
