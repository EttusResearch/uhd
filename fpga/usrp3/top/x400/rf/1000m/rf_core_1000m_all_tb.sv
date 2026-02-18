// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rf_core_1000m_all_tb.sv
//
// Description:  Summarizing all testbenches for RF core 1000 MHz

module rf_core_1000m_all_tb;

    impairment_correction_tb impairment_correction_tb_i();
    rf_core_1000m_tb rf_core_1000m_tb_i();
    dc_offset_tb dc_offset_tb_i();

endmodule
