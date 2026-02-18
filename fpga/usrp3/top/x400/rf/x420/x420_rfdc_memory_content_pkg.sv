//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x420_rfdc_memory_content_pkg
//
// Description:
//
//  This package will generate the device specific RFDC memory entries to be
//  used by the RFDC memory module.
//

package x420_rfdc_memory_content_pkg;

  import rfdc_info_pkg::*;

  function automatic rfdc_memory_entry_t get_memory_entry(
    input int i
  );

    localparam USED_ENTRIES = 12;
    rfdc_memory_entry_t entries [USED_ENTRIES] = '{
      // See coressponding code in x4xx.sv where ADC data (adc_data_out_tdata) is assigned to rx_data_qi.
      // RX side has a full tile for I/Q channel and a second tile with only one ADC for the real channel
      // DB 0 , IQ in tile 224, real mode in tile 225
      '{is_adc:1, db:0, channel:0, subchannel:1, tile:0, block:0, block_mode:I_MODE},
      '{is_adc:1, db:0, channel:0, subchannel:1, tile:0, block:1, block_mode:Q_MODE},
      '{is_adc:1, db:0, channel:0, subchannel:0, tile:1, block:0, block_mode:REAL_MODE},
      // DB 1 , IQ in tile 226, real mode in tile 227
      '{is_adc:1, db:1, channel:0, subchannel:1, tile:2, block:0, block_mode:I_MODE},
      '{is_adc:1, db:1, channel:0, subchannel:1, tile:2, block:1, block_mode:Q_MODE},
      '{is_adc:1, db:1, channel:0, subchannel:0, tile:3, block:0, block_mode:REAL_MODE},

      // See corresponding code in x4xx.sv where DAC data (dac_data_in_tdata) is assign from tx_data_qi.
      // TX side occupies a full tile for each DB
      // The first two DACs are I/Q, the fourth DAC (block 3) is real mode.
      // TX DB 0
      '{is_adc:0, db:0, channel:0, subchannel:1, tile:0, block:0, block_mode:I_MODE},
      '{is_adc:0, db:0, channel:0, subchannel:1, tile:0, block:1, block_mode:Q_MODE},
      '{is_adc:0, db:0, channel:0, subchannel:0, tile:0, block:3, block_mode:REAL_MODE},

      // TX DB 1
      '{is_adc:0, db:1, channel:0, subchannel:1, tile:1, block:0, block_mode:I_MODE},
      '{is_adc:0, db:1, channel:0, subchannel:1, tile:1, block:1, block_mode:Q_MODE},
      '{is_adc:0, db:1, channel:0, subchannel:0, tile:1, block:3, block_mode:REAL_MODE}
    };

    if (i < USED_ENTRIES) begin
      return entries[i];
    // return an disable entry for remaining entries
    end else begin
      return EMPTY_ENTRY;
    end
  endfunction

endpackage
