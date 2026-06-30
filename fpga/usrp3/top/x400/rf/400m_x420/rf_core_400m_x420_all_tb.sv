// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rf_core_400m_x420_all_tb.sv
//
// Description:  Summarizing all testbenches for RF core 400 MHz

module rf_core_400m_x420_all_tb;

    rf_core_400m_x420_tb rf_core_400m_x420_test();
    rx_dec3_tb rx_dec3_test();
    tx_inp3_tb tx_inp3_test();

endmodule
