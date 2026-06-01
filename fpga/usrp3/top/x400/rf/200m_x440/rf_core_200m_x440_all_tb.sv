// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rf_core_200m_x440_all_tb.sv
//
// Description:  Summarizing all testbenches for RF core 200M X440

module rf_core_200m_x440_all_tb;

    rf_core_200m_x440_tb rf_core_200m_x440_test();
    rx_dec3_tb rx_dec3_test();
    tx_inp3_tb tx_inp3_test();

endmodule
