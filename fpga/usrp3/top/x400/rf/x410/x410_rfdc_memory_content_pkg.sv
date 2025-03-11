//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x410_rfdc_memory_content_pkg
//
// Description:
//
//  This package will generate the device specific RFDC memory entries to be
//  used by the RFDC memory module.
//

package x410_rfdc_memory_content_pkg;

  import rfdc_info_pkg::*;

  function automatic rfdc_memory_entry_t get_memory_entry(
    input int i
  );

    localparam USED_ENTRIES = 8;
    rfdc_memory_entry_t entries [USED_ENTRIES] = '{
      // See corresponding code in x4xx.sv where ADC data (adc_data_out_tdata) is
      // assigned to rx_data_qi.
      // RX has been swapped in Rev B -> block 1 = channel 0, block 0 = channel 1
      // ADCs are located in tile 224 and 226 of the RFSoC
      '{is_adc:1, db:0, channel:0, reserved2:0, tile:0, block:1, block_mode:ENABLED},
      '{is_adc:1, db:0, channel:1, reserved2:0, tile:0, block:0, block_mode:ENABLED},
      '{is_adc:1, db:1, channel:0, reserved2:0, tile:2, block:1, block_mode:ENABLED},
      '{is_adc:1, db:1, channel:1, reserved2:0, tile:2, block:0, block_mode:ENABLED},
      // See corresponding code in x4xx.sv where DAC data (dac_data_in_tdata) is
      // assigned from tx_data_qi.
      // TX uses a 1-1 mapping.
      // DACs are the first two blocks of tile 228 and 229
      '{is_adc:0, db:0, channel:0, reserved2:0, tile:0, block:0, block_mode:ENABLED},
      '{is_adc:0, db:0, channel:1, reserved2:0, tile:0, block:1, block_mode:ENABLED},
      '{is_adc:0, db:1, channel:0, reserved2:0, tile:1, block:0, block_mode:ENABLED},
      '{is_adc:0, db:1, channel:1, reserved2:0, tile:1, block:1, block_mode:ENABLED}
    };

    if (i < USED_ENTRIES) begin
      return entries[i];
    // return an disable entry for remaining entries
    end else begin
      return EMPTY_ENTRY;
    end
  endfunction

endpackage
