#
# Copyright 2023 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import unittest
import logging
from base_tests import TestBase
from usrp_mpm.periph_manager.x4xx_clock_policy import X440ClockPolicy
from usrp_mpm.periph_manager.x4xx_clock_types import Spll1Vco
from usrp_mpm.periph_manager.x4xx_rfdc_ctrl import X4xxRfdcCtrl


class TestX440ClockConfig(TestBase):

    def test_4GHz(self):
        """
        Checking values for 4 GHz converter rate
        """
        log = logging.getLogger()
        cp = X440ClockPolicy(None, None, {}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        mcr = 2e9
        ref_clock_freq = 10e6
        clk_config = cp.get_config(ref_clock_freq, (mcr, mcr))
        self.assertEqual(clk_config.mmcm_use_defaults, False)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate([mcr,mcr]):
            self.assertTrue(clk_config.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate = clock_rate * clk_config.rfdc_configs[idx].resampling
            self.assertEqual(clk_config.rfdc_configs[idx].conv_rate, conv_rate)
        lmk_vco_rate = clk_config.spll_config.output_divider * clk_config.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n = clk_config.spll_config.pll2_prescaler * clk_config.spll_config.pll2_n_div
        self.assertEqual(lmk_vco_rate, 100e6 * pll2_n)
        mmcm_vco_rate = lmk_vco_rate / clk_config.spll_config.prc_divider * clk_config.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r0_clk'], mcr / dsp_info['spc_rx'])
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r1_clk'], mcr / dsp_info['spc_rx'])
        self.assertEqual(clk_config.spll_config.vcxo_freq, Spll1Vco.VCO100MHz)
        self.assertEqual(clk_config.spll_config.sysref_div, 1200)
        self.assertEqual(clk_config.spll_config.clkin0_r_div, 200)
        self.assertEqual(clk_config.spll_config.pll2_n_cal_div, clk_config.spll_config.pll2_n_div)
    
    def test_2_94912GHz(self):
        """
        Checking values for default 2.94912 GHz converter rate
        """
        log = logging.getLogger()
        cp = X440ClockPolicy(None, None, {}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        mcr = 368.64e6
        ref_clock_freq = 10e6
        clk_config = cp.get_config(ref_clock_freq, [mcr, mcr])
        self.assertEqual(clk_config.mmcm_use_defaults, False)
        # This should be a rate that we can achieve with PLL bypass:
        self.assertEqual(clk_config.rfdc_configs[0].conv_rate, clk_config.spll_config.output_freq)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate([mcr,mcr]):
            self.assertTrue(clk_config.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate = clock_rate * clk_config.rfdc_configs[idx].resampling
            self.assertEqual(clk_config.rfdc_configs[idx].conv_rate, conv_rate)
        lmk_vco_rate = clk_config.spll_config.output_divider * clk_config.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n = clk_config.spll_config.pll2_prescaler * clk_config.spll_config.pll2_n_div
        self.assertEqual(lmk_vco_rate, 122.88e6 * pll2_n)
        mmcm_vco_rate = lmk_vco_rate / clk_config.spll_config.prc_divider * clk_config.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r0_clk'], mcr / dsp_info['spc_rx'])
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r1_clk'], mcr / dsp_info['spc_rx'])
        self.assertEqual(clk_config.spll_config.vcxo_freq, Spll1Vco.VCO122_88MHz)
        self.assertEqual(clk_config.spll_config.sysref_div, 1152)
        self.assertEqual(clk_config.spll_config.clkin0_r_div, 250)
        self.assertEqual(clk_config.spll_config.pll2_n_cal_div, clk_config.spll_config.pll2_n_div)

    def test_253_90625M_Conv_Rate(self):
        """
        Checking values for difficult master clock rate of 253.90625 MHz
        """
        log = logging.getLogger()
        cp = X440ClockPolicy(None, None, {}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        mcr = {253.90625e6}
        ref_clock_freq = 10e6
        mcr = cp.coerce_mcr(mcr)
        clk_config = cp.get_config(ref_clock_freq, mcr)
        self.assertEqual(clk_config.mmcm_use_defaults, False)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate(mcr):
            self.assertTrue(clk_config.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate = clock_rate * clk_config.rfdc_configs[idx].resampling
            self.assertEqual(clk_config.rfdc_configs[idx].conv_rate, conv_rate)
        lmk_vco_rate = clk_config.spll_config.output_divider * clk_config.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n = clk_config.spll_config.pll2_prescaler * clk_config.spll_config.pll2_n_div
        self.assertEqual(lmk_vco_rate, 100e6 * pll2_n)
        mmcm_vco_rate = lmk_vco_rate / clk_config.spll_config.prc_divider * clk_config.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r0_clk'], mcr[0] / dsp_info['spc_rx'])
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r1_clk'], mcr[1] / dsp_info['spc_rx'])
        self.assertEqual(clk_config.spll_config.vcxo_freq, Spll1Vco.VCO100MHz)
        self.assertEqual(clk_config.spll_config.sysref_div, 6400)
        self.assertEqual(clk_config.spll_config.clkin0_r_div, 200)
        self.assertEqual(clk_config.spll_config.pll2_n_cal_div, clk_config.spll_config.pll2_n_div)

    def test_two_sample_rates(self):
        """
        Checking values for two different master clock rates
        """
        log = logging.getLogger()
        cp = X440ClockPolicy(None, None, {}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        mcr = (250e6, 1500e6)
        ref_clock_freq = 10e6
        mcr = cp.coerce_mcr(mcr)
        clk_config = cp.get_config(ref_clock_freq, mcr)
        self.assertEqual(clk_config.mmcm_use_defaults, False)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate(mcr):
            self.assertTrue(clk_config.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate = clock_rate * clk_config.rfdc_configs[idx].resampling
            self.assertEqual(clk_config.rfdc_configs[idx].conv_rate, conv_rate)
        lmk_vco_rate = clk_config.spll_config.output_divider * clk_config.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n = clk_config.spll_config.pll2_prescaler * clk_config.spll_config.pll2_n_div
        self.assertEqual(lmk_vco_rate, 100e6 * pll2_n)
        mmcm_vco_rate = lmk_vco_rate / clk_config.spll_config.prc_divider * clk_config.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r0_clk'], mcr[0] / dsp_info['spc_rx'])
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r1_clk'], mcr[1] / dsp_info['spc_rx'])
        self.assertEqual(clk_config.spll_config.vcxo_freq, Spll1Vco.VCO100MHz)
        self.assertEqual(clk_config.spll_config.sysref_div, 2400)
        self.assertEqual(clk_config.spll_config.clkin0_r_div, 200)
        self.assertEqual(clk_config.spll_config.pll2_n_cal_div, clk_config.spll_config.pll2_n_div)
    

    def test_two_sample_rates_inverted(self):
        """
        Checking that configurations are only mirrored but not completely different
        if the two MCRs are swapped
        """
        log = logging.getLogger()
        cp = X440ClockPolicy(None, None, {}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        mcr0 = [250e6, 1500e6]
        mcr1 = mcr0.copy()
        mcr1.reverse()
        ref_clock_freq = 10e6
        mcr0 = cp.coerce_mcr(mcr0)
        mcr1 = cp.coerce_mcr(mcr1)
        clk_config0 = cp.get_config(ref_clock_freq, mcr0)
        clk_config1 = cp.get_config(ref_clock_freq, mcr1)
        self.assertEqual(clk_config0.mmcm_use_defaults, False)
        self.assertEqual(clk_config1.mmcm_use_defaults, False)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate(mcr0):
            self.assertTrue(clk_config0.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate0 = clock_rate * clk_config0.rfdc_configs[idx].resampling
            conv_rate1 = mcr1[1-idx] * clk_config1.rfdc_configs[1-idx].resampling
            self.assertEqual(conv_rate0, conv_rate1)
        lmk_vco_rate = clk_config0.spll_config.output_divider * clk_config1.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n0 = clk_config0.spll_config.pll2_prescaler * clk_config0.spll_config.pll2_n_div
        pll2_n1 = clk_config1.spll_config.pll2_prescaler * clk_config1.spll_config.pll2_n_div
        self.assertEqual(pll2_n0, pll2_n1)
        mmcm_vco_rate0 = lmk_vco_rate / clk_config0.spll_config.prc_divider * clk_config0.mmcm_feedback_divider
        mmcm_vco_rate1 = lmk_vco_rate / clk_config1.spll_config.prc_divider * clk_config1.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate0, mmcm_vco_rate1)
        self.assertEqual(clk_config0.spll_config.sysref_div, clk_config1.spll_config.sysref_div)
        self.assertEqual(clk_config0.spll_config.clkin0_r_div, clk_config1.spll_config.clkin0_r_div)

        self.assertEqual(clk_config0.mmcm_output_div_map['r0_clk'], clk_config1.mmcm_output_div_map['r1_clk'])
        self.assertEqual(clk_config0.mmcm_output_div_map['r1_clk'], clk_config1.mmcm_output_div_map['r0_clk'])
        self.assertEqual(clk_config0.spll_config.vcxo_freq, clk_config1.spll_config.vcxo_freq)
        self.assertEqual(clk_config0.rfdc_configs[0], clk_config1.rfdc_configs[1])
        self.assertEqual(clk_config0.rfdc_configs[1], clk_config1.rfdc_configs[0])

    def test_two_incompatible_sample_rates(self):
        """
        Checking that for incompatible sample rates we fall back to MCR0
        """
        log = logging.getLogger()
        cp = X440ClockPolicy(None, None, {}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        mcr = (1000e6, 368.64e6)
        ref_clock_freq = 10e6
        mcr = cp.coerce_mcr(mcr)
        # ...but since values are incompatible, we can focus on #0
        clk_config = cp.get_config(ref_clock_freq, mcr)
        self.assertEqual(clk_config.mmcm_use_defaults, False)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate(mcr):
            self.assertTrue(clk_config.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate = clock_rate * clk_config.rfdc_configs[idx].resampling
            self.assertEqual(clk_config.rfdc_configs[idx].conv_rate, conv_rate)
        lmk_vco_rate = clk_config.spll_config.output_divider * clk_config.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n = clk_config.spll_config.pll2_prescaler * clk_config.spll_config.pll2_n_div
        self.assertEqual(lmk_vco_rate, 100e6 * pll2_n)
        mmcm_vco_rate = lmk_vco_rate / clk_config.spll_config.prc_divider * clk_config.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r0_clk'], mcr[0] / dsp_info['spc_rx'])
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r1_clk'], mcr[1] / dsp_info['spc_rx'])
        self.assertEqual(clk_config.spll_config.vcxo_freq, Spll1Vco.VCO100MHz)
        self.assertEqual(clk_config.spll_config.sysref_div, 1200)
        self.assertEqual(clk_config.spll_config.clkin0_r_div, 200)
        self.assertEqual(clk_config.spll_config.pll2_n_cal_div, clk_config.spll_config.pll2_n_div)

    def test_converter_rate_arg(self):
        """
        Checks if the converter_rate argument is used.
        """
        log = logging.getLogger()
        cr = 2e9
        cp = X440ClockPolicy(None, None, {'converter_rate':cr}, log)
        dsp_info = {
            'num_rx_chans': 4,
            'num_tx_chans': 4,
            'bw': 1600,
            'extra_resampling': 1,
            'spc_rx': 8,
            'spc_tx': 8,
        }
        cp.set_dsp_info([dsp_info, dsp_info])
        # In theory, for MCR=500e6, the Conv_Rate should go to 4e9. By passing the conv_rate
        # argument we should be able to influence this.
        mcr = (500e6,)
        ref_clock_freq = 10e6
        mcr = cp.coerce_mcr(mcr)
        clk_config = cp.get_config(ref_clock_freq, mcr)
        self.assertEqual(clk_config.mmcm_use_defaults, False)
        # We need to calculate if the values lead to what we want
        for idx, clock_rate in enumerate(mcr):
            self.assertTrue(clk_config.rfdc_configs[idx].resampling in X4xxRfdcCtrl.RFDC_RESAMPLER)
            conv_rate = clock_rate * clk_config.rfdc_configs[idx].resampling
            self.assertEqual(conv_rate, cr)
            self.assertEqual(clk_config.rfdc_configs[idx].conv_rate, conv_rate)
        lmk_vco_rate = clk_config.spll_config.output_divider * clk_config.spll_config.output_freq
        # Overall PLL2 N divider as combination of prescalar and n div
        pll2_n = clk_config.spll_config.pll2_prescaler * clk_config.spll_config.pll2_n_div
        self.assertEqual(lmk_vco_rate, 100e6 * pll2_n)
        mmcm_vco_rate = lmk_vco_rate / clk_config.spll_config.prc_divider * clk_config.mmcm_feedback_divider
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r0_clk'], mcr[0] / dsp_info['spc_rx'])
        self.assertEqual(mmcm_vco_rate / clk_config.mmcm_output_div_map['r1_clk'], mcr[1] / dsp_info['spc_rx'])
        self.assertEqual(clk_config.spll_config.vcxo_freq, Spll1Vco.VCO100MHz)
        self.assertEqual(clk_config.spll_config.sysref_div, 1200)
        self.assertEqual(clk_config.spll_config.clkin0_r_div, 200)
        self.assertEqual(clk_config.spll_config.pll2_n_cal_div, clk_config.spll_config.pll2_n_div)
