#
# Copyright 2022 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4xx Clocking Policies

These clocking policies are sets of rules for configuring the various clocks on
X4xx motherboards.
"""
import math
from collections import OrderedDict
from dataclasses import dataclass, field
from usrp_mpm.mpmutils import parse_multi_device_arg
from usrp_mpm.periph_manager.x4xx_clock_types import Spll1Vco
from usrp_mpm.dboard_manager import ZBX


###############################################################################
# Define Dataclasses for component settings
###############################################################################
# pylint: disable=too-many-instance-attributes
@dataclass
class RfdcConfig:
    """
    Provide all relevant RFdc settings.

    Since we can have different settings per daughterboard, we have one of these
    per daughterboard.
    """
    # The frequency that the RFDC PLL will output
    conv_rate: float
    # This is both the interpolation and decimation factor. Because we always
    # have one radio block per daughterboard, the radio rates for Tx and Rx are
    # always identical, and thus the resampling factor is the same, too.
    resampling: int = 1
    # If these latency values are set to None, then we auto-detect them. If they
    # are an integer value, they are applied as-is to all ADCs/DACs, respectively,
    # that are associated with this daughterboard's RfdcConfig.
    adc_latency: int = None
    dac_latency: int = None


@dataclass
class SpllConfig:
    """
    Provide all relevant SPLL settings.
    """
    # The reference frequency for the SPLL (e.g. from the external reference input,
    # often 10 MHz)
    ref_freq: float
    # The frequency that is generated at the SPLL output for the ADC/DACs. In
    # other words, the reference frequency for the RFDC PLLs.
    output_freq: float
    # Output divider for the ADC/DAC clock signal
    output_divider: int
    # The output divider for the PRC output (PRC is thus PLL2 VCO rate divided
    # by this)
    prc_divider: int
    vcxo_freq: Spll1Vco
    sysref_div: int
    clkin0_r_div: int
    pll1_n_div: int
    pll2_prescaler: int
    pll2_n_cal_div: int
    pll2_n_div: int

@dataclass
class X4xxClockConfig:
    """
    Stores all clock-related settings to achieve a requested master clock rate.
    Consolidates settings for the main PLL (SPLL, the LMK04832) as well as for
    the RFDC and MMCM.
    """
    spll_config: SpllConfig
    rfdc_configs: list # list of RfdcConfig
    # If bitfile defaults should be used set to true
    mmcm_use_defaults: bool
    # The feedback divider of the MMCM
    mmcm_feedback_divider: int = 1
    # The output divider of the MMCM (dict of clock name and div value)
    mmcm_output_div_map: dict = field(default_factory=dict)
    # Input divider of the MMCM
    mmcm_input_divider: int = 1

def lcm(x, y):
    """
    Least common multiple, can be taken from math if we upgrade to Python >= 3.9.0
    """
    x = int(x)
    y = int(y)
    return int(x * y / math.gcd(x, y))

# pylint: enable=too-many-instance-attributes

class X4xxClockPolicy:
    """
    Base class for X4xx clock policies.

    Such a policy would be used by the X4xxClockManager to determine settings.
    """
    def __init__(self, args, log):
        self._initial_args = args
        self.args = args

    def set_dsp_info(self, dsp_info):
        """
        Store the DSP info of the current FPGA image.

        The individual clock policies can choose what to do with this info.
        """
        raise NotImplementedError()

    def get_default_mcr(self):
        """
        Return a reasonable default master clock rate. This method is called
        during initialization, to decide which rate to configure the device to,
        when no MCR is given by the user.

        This method is called twice: Once before set_dsp_info() is called, and
        once afterwards. The first time a valid, sensible clock rate must be
        returned which can be used to initialize the device enough to enable
        the FPGA. When the FPGA is running, the DSP info can be read back and
        then this function is queried again, because the first time around, we
        may have chosen an MCR value that is not useful for the capabilities of
        this particular bitfile. It is therefore valid to return different values
        depending on whether or not the DSP info is set.
        """
        raise NotImplementedError()

    def get_num_rates(self):
        """
        Returns the number of different master clock rates we can handle. If it
        is a value greater than one, then we need to be able to handle vectors
        of MCRs in the following functions.
        """
        raise NotImplementedError()

    def validate_ref_clock_freq(self, ref_clock_freq, master_clock_rates):
        """
        Verify that ref_clock_freq is a valid reference clock frequency for the
        given master clock rates.

        Throw a RuntimeError if not.

        Must not modify the state of this policy.
        """
        raise NotImplementedError()

    def coerce_mcr(self, master_clock_rates):
        """
        Validate that the requested master clock rate is valid.

        May coerce the master clock rates if not, but may also throw a ValueError
        if there is no reasonable way to coerce the desired rates.

        Must not modify the state of this policy.
        """
        raise NotImplementedError()

    def get_config(self, ref_clock_freq, master_clock_rates):
        """
        This is where the action happens: Generate the clock configuration.

        This will return a X4xxClockConfig class which can be used by X4xxClockMgr
        to actually configure various clocks.

        This method may change the state of this class.
        """
        raise NotImplementedError()

class X410ClockPolicy(X4xxClockPolicy):
    """
    This is a pretty dumb policy, everything is hardcoded.
    Some properties:

    - We only allow 3 different converter rates
    - RFdc PLL is unused
    - Only one rate is allowed (same rate on both dboards)
    """

    DEFAULT_MASTER_CLOCK_RATE = 122.88e6 # Keep this a low value

    master_to_sample_clk = OrderedDict({
        #      MCR:    (DSP BW, SPLL, decim, legacy mode)
        122.88e6*4:    (400, 2.94912e9, 2, False), # RF (1M-8G)
        122.88e6*2:    (200, 2.94912e9, 2, False), # RF (1M-8G)
        122.88e6*1:    (100, 2.94912e9, 8, False), # RF (1M-8G)
        125e6*1:       (100, 3.00000e9, 8, False), # RF (1M-8G)
        125e6*2:       (200, 3.00000e9, 2, False), # RF (1M-8G)
        125e6*4:       (400, 3.00000e9, 2, False), # RF (1M-8G)
        200e6:         (400, 3.00000e9, 4, True ), # RF (Legacy Mode)
    })

    def __init__(self, mboard_info, dboard_infos, args, log):
        super().__init__(args, log)
        self._dsp_info = None
        self._dsp_bw = None

    def set_dsp_info(self, dsp_info):
        """
        Store the DSP info of the current FPGA image.
        """
        self._dsp_info = dsp_info
        self._dsp_bw = dsp_info[0]['bw']
        assert self._dsp_bw in [x[0] for x in self.master_to_sample_clk.values()]

    def get_default_mcr(self):
        """
        Return a reasonable default master clock rate.
        """
        if self._dsp_bw is None:
            return self.DEFAULT_MASTER_CLOCK_RATE
        for mcr, cfg in self.master_to_sample_clk.items():
            if cfg[0] == self._dsp_bw:
                return mcr
        raise AssertionError("Cannot determine default MCR.")

    # pylint: disable=no-self-use
    def get_num_rates(self):
        """
        Returns the number of different master clock rates we can handle.
        """
        return 1

    # pylint: enable=no-self-use

    def validate_ref_clock_freq(self, ref_clock_freq, master_clock_rates):
        """
        Verify that ref_clock_freq is a valid reference clock frequency for the
        given master clock rates.

        Will throw a RuntimeError if not.

        Does not modify the state of this policy.
        """
        self.coerce_mcr(master_clock_rates)
        mcr = master_clock_rates[0]
        is_legacy_mode = self.master_to_sample_clk[mcr][3]
        step_size = 50e3 if is_legacy_mode else 40e3
        if ref_clock_freq % step_size != 0:
            raise RuntimeError(
                'External reference clock frequency is of incorrect step size.')

    def coerce_mcr(self, master_clock_rates):
        """
        Validate that the requested master clock rate is valid.

        Will throw a ValueError if not.

        Does not modify the state of this policy.
        """
        assert len(master_clock_rates) == 1
        mcr = master_clock_rates[0]
        if mcr not in self.master_to_sample_clk or \
                (self._dsp_bw and self.master_to_sample_clk[mcr][0] != self._dsp_bw):
            raise ValueError(
                f"Invalid master clock rate: {mcr/1e6} MHz for current FPGA image "
                f"with bandwidth {self._dsp_bw} MHz!")
        return master_clock_rates


    def get_config(self, ref_clock_freq, master_clock_rates):
        """
        This is where the action happens: Generate the clock configuration.
        """
        assert ref_clock_freq
        assert master_clock_rates
        self.coerce_mcr(master_clock_rates)
        mcr = master_clock_rates[0]
        rfdc_freq = self.master_to_sample_clk[mcr][1]
        legacy_mode = self.master_to_sample_clk[mcr][3]
        pfd1 = {2.94912e9: 40e3, 3e9: 50e3, 3.072e9: 40e3}[rfdc_freq]
        spll_args = {
            2.94912e9: {
                'ref_freq': ref_clock_freq,
                'output_freq': rfdc_freq,
                'output_divider': 1,
                'prc_divider': 0x3C if legacy_mode else 0x30,
                'vcxo_freq': Spll1Vco.VCO122_88MHz,
                'sysref_div': 1152,
                'clkin0_r_div': int(ref_clock_freq / pfd1),
                'pll1_n_div': 64,
                'pll2_prescaler': 2,
                'pll2_n_cal_div': 12,
                'pll2_n_div': 12,
            },
            3e9: {
                'ref_freq': ref_clock_freq,
                'output_freq': rfdc_freq,
                'output_divider': 1,
                'prc_divider': 0x3C if legacy_mode else 0x30,
                'vcxo_freq': Spll1Vco.VCO100MHz,
                'sysref_div': 1200,
                'clkin0_r_div': int(ref_clock_freq / pfd1),
                'pll1_n_div': 50,
                'pll2_prescaler': 3,
                'pll2_n_cal_div': 10,
                'pll2_n_div': 10,
            },
            3.072e9: {
                'ref_freq': ref_clock_freq,
                'output_freq': rfdc_freq,
                'output_divider': 1,
                'prc_divider': 0x3C if legacy_mode else 0x30,
                'vcxo_freq': Spll1Vco.VCO122_88MHz,
                'sysref_div': 1200,
                'clkin0_r_div': int(ref_clock_freq / pfd1),
                'pll1_n_div': 64,
                'pll2_prescaler': 5,
                'pll2_n_cal_div': 5,
                'pll2_n_div': 5,
            },
        }[rfdc_freq]
        spll_config = SpllConfig(**spll_args)
        resampling_factor = self.master_to_sample_clk[mcr][2]
        rfdc_config = RfdcConfig(
            conv_rate=rfdc_freq,
            resampling=resampling_factor,
            # Note: Those latency values were experimentally determined using
            # the algorith shown in X4xxRfdcCtrl.determine_tile_latency(). We
            # should be not hard coding this in accordance with pg269, but since
            # these numbers have worked since the initial release of X410, we
            # won't change that now to minimize risk.
            adc_latency=1272,
            dac_latency=816,
        )
        return X4xxClockConfig(
            spll_config=spll_config,
            rfdc_configs=[rfdc_config, rfdc_config],
            mmcm_use_defaults=True
        )

def get_clock_policy(mboard_info, dboard_infos, args, log):
    """
    Return a clocking policy object based on the available hardware.
    """
    if dboard_infos[0]['pid'] in ZBX.pids:
        return X410ClockPolicy(mboard_info, dboard_infos, args, log)
    raise RuntimeError("Could not resolve clock policy.")
