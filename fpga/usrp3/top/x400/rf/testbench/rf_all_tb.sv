//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_all_tb
//
// Description:
//
//   Top-level testbench for X400 RF components. This instantiates all the RF
//   testbenches.
//

module rf_all_tb;
  `include "test_exec.svh"
  import PkgTestExec::*;

  tb_adc_gearbox_2x1     tb_adc_gearbox_2x1_i     ();
  tb_adc_gearbox_2x4     tb_adc_gearbox_2x4_i     ();
  tb_adc_gearbox_8x4     tb_adc_gearbox_8x4_i     ();
  tb_capture_sysref      tb_capture_sysref_i      ();
  tb_dac_gearbox_12x8    tb_dac_gearbox_12x8_i    ();
  tb_dac_gearbox_4x2     tb_dac_gearbox_4x2_i     ();
  tb_dac_gearbox_6x12    tb_dac_gearbox_6x12_i    ();
  tb_ddc_400m_saturate   tb_ddc_400m_saturate_i   ();
  tb_duc_400m_saturate   tb_duc_400m_saturate_i   ();
  tb_rf_nco_reset        tb_rf_nco_reset_i        ();
  tb_rf_reset_controller tb_rf_reset_controller_i ();

  initial begin
    test.start_tb("rf_all_tb", 1ms);

    test.start_test("Run RF TBs");
    forever begin
      #100ns;
      if (
        tb_adc_gearbox_2x1_i.StopSim     &&
        tb_adc_gearbox_2x4_i.StopSim     &&
        tb_adc_gearbox_8x4_i.StopSim     &&
        tb_capture_sysref_i.StopSim      &&
        tb_dac_gearbox_12x8_i.StopSim    &&
        tb_dac_gearbox_4x2_i.StopSim     &&
        tb_dac_gearbox_6x12_i.StopSim    &&
        tb_ddc_400m_saturate_i.StopSim   &&
        tb_duc_400m_saturate_i.StopSim   &&
        tb_rf_nco_reset_i.StopSim        &&
        tb_rf_reset_controller_i.StopSim
      ) break;
    end
    test.end_test();

    // If they all stop before the timeout, and there are no errors, then we
    // assume everything passed.
    test.end_tb();
  end

endmodule : rf_all_tb
