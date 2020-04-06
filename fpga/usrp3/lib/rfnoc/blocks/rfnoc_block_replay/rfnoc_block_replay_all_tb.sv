//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_replay_tb
//
// Description:
//
//   This testbench is the top-level testbench for the RFnoC Replay block. It
//   instantiates several different variants of the Replay testbench, each
//   using different parameters, to test different configurations.
//

`default_nettype none


module rfnoc_block_replay_all_tb;

  `include "test_exec.svh"
  import PkgTestExec::*;


  //---------------------------------------------------------------------------
  // Test Definitions
  //---------------------------------------------------------------------------

  typedef struct {
    int CHDR_W;
    int ITEM_W;
    int NUM_PORTS;
    int MEM_DATA_W;
    int MEM_ADDR_W;
    int TEST_REGS;
    int TEST_FULL;
    int STALL_PROB;
  } test_config_t;

  localparam NUM_TESTS = 15;

  localparam test_config_t test[NUM_TESTS] = '{
    // Test different CHDR and memory widths:
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W:  32, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 1, STALL_PROB: 25},
    '{CHDR_W: 128, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W:  32, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 0, STALL_PROB: 25},
    '{CHDR_W: 256, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W:  32, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 1, STALL_PROB: 25},
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 2, MEM_DATA_W:  64, MEM_ADDR_W: 16, TEST_REGS: 1, TEST_FULL: 1, STALL_PROB: 25},
    '{CHDR_W: 128, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W:  64, MEM_ADDR_W: 16, TEST_REGS: 1, TEST_FULL: 0, STALL_PROB: 25},
    '{CHDR_W: 256, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W:  64, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 0, STALL_PROB: 25},
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W: 128, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 0, STALL_PROB: 25},
    '{CHDR_W: 128, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W: 128, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 0, STALL_PROB: 25},
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W: 256, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 1, STALL_PROB: 25},
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 1, MEM_DATA_W: 512, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 0, STALL_PROB: 25},
    // Test different stall probabilities:
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 2, MEM_DATA_W:  64, MEM_ADDR_W: 16, TEST_REGS: 1, TEST_FULL: 1, STALL_PROB:  0},
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 2, MEM_DATA_W:  64, MEM_ADDR_W: 16, TEST_REGS: 1, TEST_FULL: 1, STALL_PROB: 75},
    // Test large memory (> 32-bit) to check 64-bit registers:
    '{CHDR_W:  64, ITEM_W: 32, NUM_PORTS: 2, MEM_DATA_W:  64, MEM_ADDR_W: 34, TEST_REGS: 1, TEST_FULL: 0, STALL_PROB:  0},
    // Test different item widths to check time is handled correctly
    '{CHDR_W:  64, ITEM_W: 16, NUM_PORTS: 1, MEM_DATA_W:  32, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 1, STALL_PROB: 25},
    '{CHDR_W: 256, ITEM_W:  8, NUM_PORTS: 1, MEM_DATA_W:  32, MEM_ADDR_W: 16, TEST_REGS: 0, TEST_FULL: 1, STALL_PROB: 25}
  };


  //---------------------------------------------------------------------------
  // DUT Instances
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_TESTS; i++) begin : gen_test_config
    rfnoc_block_replay_tb #(
      .CHDR_W     (test[i].CHDR_W    ),
      .NUM_PORTS  (test[i].NUM_PORTS ),
      .MEM_DATA_W (test[i].MEM_DATA_W),
      .MEM_ADDR_W (test[i].MEM_ADDR_W),
      .TEST_FULL  (test[i].TEST_FULL )
    ) rfnoc_block_replay_tb_i ();
  end : gen_test_config

endmodule : rfnoc_block_replay_all_tb


`default_nettype wire
