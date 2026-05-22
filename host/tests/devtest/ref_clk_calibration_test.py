#!/usr/bin/env python3
#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Test the ref_clk_calibration feature."""

from uhd_test_base import UHDPythonTestCase


class RefClkCalibrationTest(UHDPythonTestCase):
    """Test ref_clk_calibration feature.

    Reads the current tuning word and writes it back. Does not call
    store_ref_clk_tuning_word to avoid modifying EEPROM during the test.
    """

    test_name = "RefClkCalibrationTest"

    def run_test(self, test_name, test_args):
        """Run test and report results."""
        import uhd

        usrp = uhd.usrp.MultiUSRP(self.args_str)
        ref_cal = usrp.get_mb_controller().get_ref_clk_calibration()
        word = ref_cal.get_ref_clk_tuning_word()
        self.log.info("Current ref_clk tuning word: 0x%X", word)
        ref_cal.set_ref_clk_tuning_word(word)

        return {"passed": True}
