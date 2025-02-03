#
# Copyright 2022 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""X4xx Clocking Policies.

These clocking policies are sets of rules for configuring the various clocks on
X4xx motherboards.
"""
import math
from collections import OrderedDict
from dataclasses import dataclass, field

from usrp_mpm.chips import LMK04832
from usrp_mpm.dboard_manager import FBX, ZBX
from usrp_mpm.mpmutils import parse_multi_device_arg
from usrp_mpm.periph_manager.x4xx_clock_lookup import MCR_LMK_VCO, RFDC_PLL_CONFIGS
from usrp_mpm.periph_manager.x4xx_clock_types import Spll1Vco
from usrp_mpm.periph_manager.x4xx_rfdc_ctrl import X4xxRfdcCtrl
from usrp_mpm.periph_manager.x4xx_sample_pll import LMK04832X4xx


###############################################################################
# Define Dataclasses for component settings
###############################################################################
# pylint: disable=too-many-instance-attributes
@dataclass
class RfdcConfig:
    """Provide all relevant RFDC settings.

    Since we can have different settings per daughterboard, we have one of these
    per daughterboard.
    """

    # The frequency that the RFDC PLL will output
    conv_rate: float
    # This is both the interpolation and decimation factor. Because we always
    # have one radio block per daughterboard, the radio rates for Tx and Rx are
    # always identical, and thus the resampling factor is the same, too.
    resampling: int = 1


@dataclass
class SpllConfig:
    """Provide all relevant SPLL settings."""

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
    sysref_delay: int
    clkin0_r_div: int
    pll1_n_div: int
    pll2_prescaler: int
    pll2_n_cal_div: int
    pll2_n_div: int
    # PRC output to the daughterboard
    prc_to_db: bool


@dataclass
class X4xxClockConfig:
    """Stores all clock-related settings to achieve a requested master clock rate.

    Consolidates settings for the main PLL (SPLL, the LMK04832) as well as for
    the RFDC and MMCM.
    """

    spll_config: SpllConfig
    rfdc_configs: list  # list of RfdcConfig
    # If bitfile defaults should be used set to true
    mmcm_use_defaults: bool
    # The feedback divider of the MMCM
    mmcm_feedback_divider: int = 1
    # The output divider of the MMCM (dict of clock name and div value)
    mmcm_output_div_map: dict = field(default_factory=dict)
    # Input divider of the MMCM
    mmcm_input_divider: int = 1


def lcm(x, y):
    """Least common multiple.

    This can be taken from math if we upgrade to Python >= 3.9.0
    """
    x = int(x)
    y = int(y)
    return int(x * y / math.gcd(x, y))


# pylint: enable=too-many-instance-attributes


class X4xxClockPolicy:
    """Base class for X4xx clock policies.

    Such a policy would be used by the X4xxClockManager to determine settings.
    """

    def __init__(self, args, log):
        """Initialize the X4xx clock policy."""
        self._initial_args = args
        self.args = args

    def set_dsp_info(self, dsp_info):
        """Store the DSP info of the current FPGA image.

        The individual clock policies can choose what to do with this info.
        """
        raise NotImplementedError()

    def get_default_mcr(self):
        """Return a reasonable default master clock rate.

        This method is called during initialization, to decide which rate to
        configure the device to, when no MCR is given by the user.

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

    def get_radio_clock_rate(self, mcr):
        """Return the radio clock rates for the current configuration.

        The individual clock policies can choose how to calculate this info.
        """
        raise NotImplementedError()

    def get_num_rates(self):
        """Returns the number of different master clock rates we can handle.

        If it is a value greater than one, then we need to be able to
        handle vectors of MCRs in the following functions.
        """
        raise NotImplementedError()

    def validate_ref_clock_freq(self, ref_clock_freq, master_clock_rates):
        """Verify that ref_clock_freq is a valid reference clock frequency.

        Checks whether ref_clock_freq is valid for the given master clock rates.

        Will throw a RuntimeError if not.

        Does not modify the state of this policy.
        """

    def get_intermediate_clk_settings(self, ref_clk_freq, old_mcrs, new_mcrs):
        """Returns an intermediate clock settings object.

        An  intermediate object is necessary if going from the old to
        the new master clock rates would fail otherwise.
        """
        raise NotImplementedError()

    def coerce_mcr(self, master_clock_rates):
        """Validate that the requested master clock rate is valid.

        May coerce the master clock rates if not, but may also throw a ValueError
        if there is no reasonable way to coerce the desired rates.

        Must not modify the state of this policy.
        """
        raise NotImplementedError()

    def get_config(self, ref_clock_freq, master_clock_rates):
        """Generate the clock configuration (This is where the action happens).

        This will return a X4xxClockConfig class which can be used by X4xxClockMgr
        to actually configure various clocks.

        This method may change the state of this class.
        """
        raise NotImplementedError()

    def should_reboot_on_reconfiguration(self):
        """This returns if MPM should reboot on a clocking reconfiguration."""
        raise NotImplementedError()


class X410ClockPolicy(X4xxClockPolicy):
    """This is the clocking policy for X440.

    This is a pretty simple, everything is hardcoded.
    Some properties:

    - We only allow 3 different converter rates
    - RFdc PLL is unused
    - Only one rate is allowed (same rate on both dboards)
    """

    DEFAULT_MASTER_CLOCK_RATE = 122.88e6  # Keep this a low value

    master_to_sample_clk = OrderedDict(
        {
            #      MCR:    (DSP BW, SPLL, decim, legacy mode)
            122.88e6 * 4: (400, 2.94912e9, 2, False),  # RF (1M-8G)
            122.88e6 * 2: (200, 2.94912e9, 2, False),  # RF (1M-8G)
            122.88e6 * 1: (100, 2.94912e9, 8, False),  # RF (1M-8G)
            125e6 * 1: (100, 3.00000e9, 8, False),  # RF (1M-8G)
            125e6 * 2: (200, 3.00000e9, 2, False),  # RF (1M-8G)
            125e6 * 4: (400, 3.00000e9, 2, False),  # RF (1M-8G)
            200e6: (400, 3.00000e9, 4, True),  # RF (Legacy Mode)
        }
    )

    def __init__(self, mboard_info, dboard_infos, args, log):
        """Initialize the X410 clock policy."""
        super().__init__(args, log)
        self._dsp_info = None
        self._dsp_bw = None

    def set_dsp_info(self, dsp_info):
        """Store the DSP info of the current FPGA image."""
        self._dsp_info = dsp_info
        if self._dsp_info[0]["num_rx_chans"] == 0 and self._dsp_info[0]["num_tx_chans"] == 0:
            return

        # this part of DSP initialization is only relevant for radio frontends
        self._dsp_bw = dsp_info[0]["bw"]
        assert self._dsp_bw in [x[0] for x in self.master_to_sample_clk.values()]

    def get_default_mcr(self):
        """Return a reasonable default master clock rate."""
        if self._dsp_bw is None:
            return self.DEFAULT_MASTER_CLOCK_RATE
        for mcr, cfg in self.master_to_sample_clk.items():
            if cfg[0] == self._dsp_bw:
                return mcr
        raise AssertionError("Cannot determine default MCR.")

    def get_radio_clock_rate(self, mcr):
        """Return the radio clock rates for the current configuration."""
        return mcr / self._dsp_info[0]["spc_rx"]

    # pylint: disable=no-self-use
    def get_num_rates(self):
        """Returns the number of different master clock rates we can handle."""
        return 1

    # pylint: enable=no-self-use

    def validate_ref_clock_freq(self, ref_clock_freq, master_clock_rates):
        """Verify that ref_clock_freq is a valid reference clock frequency.

        Checks whether ref_clock_freq is valid for the given master clock rates.

        Will throw a RuntimeError if not.

        Does not modify the state of this policy.
        """
        self.coerce_mcr(master_clock_rates)
        mcr = master_clock_rates[0]
        is_legacy_mode = self.master_to_sample_clk[mcr][3]
        step_size = 50e3 if is_legacy_mode else 40e3
        if ref_clock_freq % step_size != 0:
            raise RuntimeError("External reference clock frequency is of incorrect step size.")

    def get_intermediate_clk_settings(self, ref_clk_freq, old_mcrs, new_mcrs):
        """Returns an intermediate clock settings object.

        An  intermediate object is necessary if going from the old to
        the new master clock rates would fail otherwise.
        """
        return None

    def coerce_mcr(self, master_clock_rates):
        """Validate that the requested master clock rate is valid.

        Will throw a ValueError if not.

        Does not modify the state of this policy.
        """
        assert len(master_clock_rates) == 1
        mcr = master_clock_rates[0]
        if mcr not in self.master_to_sample_clk or (
            self._dsp_bw and self.master_to_sample_clk[mcr][0] != self._dsp_bw
        ):
            raise ValueError(
                f"Invalid master clock rate: {mcr/1e6} MHz for current FPGA image "
                f"with bandwidth {self._dsp_bw} MHz!"
            )
        return master_clock_rates

    def get_config(self, ref_clock_freq, master_clock_rates):
        """This is where the action happens: Generate the clock configuration."""
        assert ref_clock_freq
        assert master_clock_rates
        self.coerce_mcr(master_clock_rates)
        mcr = master_clock_rates[0]
        rfdc_freq = self.master_to_sample_clk[mcr][1]
        legacy_mode = self.master_to_sample_clk[mcr][3]
        pfd1 = {2.94912e9: 40e3, 3e9: 50e3, 3.072e9: 40e3}[rfdc_freq]
        spll_args = {
            2.94912e9: {
                "ref_freq": ref_clock_freq,
                "output_freq": rfdc_freq,
                "output_divider": 1,
                "prc_divider": 0x3C if legacy_mode else 0x30,
                "vcxo_freq": Spll1Vco.VCO122_88MHz,
                "sysref_div": 1152,
                "sysref_delay": 32,
                "clkin0_r_div": int(ref_clock_freq / pfd1),
                "pll1_n_div": 64,
                "pll2_prescaler": 2,
                "pll2_n_cal_div": 12,
                "pll2_n_div": 12,
                "prc_to_db": True,
            },
            3e9: {
                "ref_freq": ref_clock_freq,
                "output_freq": rfdc_freq,
                "output_divider": 1,
                "prc_divider": 0x3C if legacy_mode else 0x30,
                "vcxo_freq": Spll1Vco.VCO100MHz,
                "sysref_div": 1200,
                "sysref_delay": 32,
                "clkin0_r_div": int(ref_clock_freq / pfd1),
                "pll1_n_div": 50,
                "pll2_prescaler": 3,
                "pll2_n_cal_div": 10,
                "pll2_n_div": 10,
                "prc_to_db": True,
            },
            3.072e9: {
                "ref_freq": ref_clock_freq,
                "output_freq": rfdc_freq,
                "output_divider": 1,
                "prc_divider": 0x3C if legacy_mode else 0x30,
                "vcxo_freq": Spll1Vco.VCO122_88MHz,
                "sysref_div": 1200,
                "sysref_delay": 32,
                "clkin0_r_div": int(ref_clock_freq / pfd1),
                "pll1_n_div": 64,
                "pll2_prescaler": 5,
                "pll2_n_cal_div": 5,
                "pll2_n_div": 5,
                "prc_to_db": True,
            },
        }[rfdc_freq]
        spll_config = SpllConfig(**spll_args)
        resampling_factor = self.master_to_sample_clk[mcr][2]
        rfdc_config = RfdcConfig(conv_rate=rfdc_freq, resampling=resampling_factor)
        return X4xxClockConfig(
            spll_config=spll_config, rfdc_configs=[rfdc_config, rfdc_config], mmcm_use_defaults=True
        )

    def should_reboot_on_reconfiguration(self):
        """MPM reboot is not necessary for X410."""
        return False


class X440ClockPolicy(X4xxClockPolicy):
    """This is the clocking policy for X440.

    In contrast to the clocking policy of the X410 it uses the RFDC PLL
    and MMCM and it uses the flexibility of the LMK04832 much more.
    """

    # This is the lowest value possible and should therefore be
    # achievable with every bitfile. This value will only used
    # if we don't have information about the DSP bandwidth of
    # the bitfile available, otherwise the lookup table
    # `bandwidth_to_default_mcr` will be used.
    DEFAULT_MASTER_CLOCK_RATE = 125e6

    # Lookup table for setting up the correct master clock rate
    # depending on the DSP bandwidth (in MHz) of the FPGA image.
    bandwidth_to_default_mcr = {
        200: 245.76e6,
        400: 368.64e6,
        1600: 368.64e6,
    }

    def __init__(self, mboard_info, dboard_infos, args, log):
        """Initialize the X440 clock policy."""
        self.log = log.getChild("Clk_Policy")
        self._initial_args = args
        self.args = args
        self.conv_rates = None
        self._read_converter_rates()

        self._dsp_info = None
        # Use default values during startup
        self._dsp_bw = None
        self._spc = 8
        self._extra_resampling = 1
        self._valid_sysref_freqs = list(
            sysref_setting["SYSREF_FREQ"]
            for vcxo in LMK04832X4xx.SYSREF_CONFIG.keys()
            for sysref_setting in LMK04832X4xx.SYSREF_CONFIG[vcxo]
        )

    def set_dsp_info(self, dsp_info):
        """Store the DSP info of the current FPGA image."""
        self._dsp_info = dsp_info
        if self._dsp_info[0]["num_rx_chans"] == 0 and self._dsp_info[0]["num_tx_chans"] == 0:
            return

        # this part of DSP initialization is only relevant for radio frontends
        self._dsp_bw = dsp_info[0]["bw"]
        # We assume that SPC == the bitfile's spc_rx
        self._spc = dsp_info[0]["spc_rx"]
        # Assume that the SPC will always be the same for both DBs
        assert self._spc == dsp_info[1]["spc_rx"]
        self._extra_resampling = dsp_info[0]["extra_resampling"]
        # Assume that the extra resampling will always be the same for both DBs
        assert self._extra_resampling == dsp_info[1]["extra_resampling"]

    def get_default_mcr(self):
        """Return a reasonable default master clock rate."""
        if self._dsp_bw is None:
            return self.DEFAULT_MASTER_CLOCK_RATE
        if self.bandwidth_to_default_mcr.get(self._dsp_bw):
            return self.bandwidth_to_default_mcr.get(self._dsp_bw)
        raise AssertionError("Cannot determine default MCR.")

    def get_radio_clock_rate(self, mcr):
        """Return the radio clock rates for the current configuration."""
        return mcr / self._spc

    def get_num_rates(self):
        """Returns the number of different master clock rates we can handle."""
        return 2

    def validate_ref_clock_freq(self, ref_clock_freq, master_clock_rates):
        """Verify that ref_clock_freq is a valid reference clock frequency.

        Checks whether ref_clock_freq is valid for the given master clock rates.

        Will throw a RuntimeError if not.

        Does not modify the state of this policy.
        """
        master_clock_rates = self.coerce_mcr(master_clock_rates)
        config = self.get_config(ref_clock_freq, master_clock_rates)
        sysref_config = next(
            sysref_setting
            for sysref_setting in LMK04832X4xx.SYSREF_CONFIG[
                100e6 if config.spll_config.vcxo_freq == Spll1Vco.VCO100MHz else 122.88e6
            ]
            if sysref_setting["SYSREF_FREQ"]
            in self._find_sysref_matches(
                config.rfdc_configs[0].conv_rate / config.rfdc_configs[0].resampling
            )
        )

        step_size = sysref_config["PDF"]

        if ref_clock_freq % step_size != 0:
            raise RuntimeError("External reference clock frequency is of incorrect step size.")

    def _read_converter_rates(self):
        """Gets the converter rate.

        Gets the converter rate  from the argument string, if it is given there,
        None otherwise.
        """
        self.conv_rates = self.args.get("converter_rate")
        if self.conv_rates is None:
            return
        self.conv_rates = list(parse_multi_device_arg(self.args["converter_rate"], conv=float))
        if len(self.conv_rates) == 1:
            self.conv_rates = [self.conv_rates[0]] * self.get_num_rates()
        elif len(self.conv_rates) != self.get_num_rates():
            raise RuntimeError(
                f"Invalid number of converter rates provided! Must provide either a "
                f"single rate or {self.get_num_rates()} rates (one per daughterboard)."
            )

    def _match_bypass_freqs(self, mcrs):
        """Analyzes if the two chosen sample rates can be generated by using the RFDC PLL bypass."""
        use_pll_bypass = False

        # Check if both DBs require the same LMK VCO rate (otherwise we can return here already).
        common_vco_rates = self._get_common_lmk_vco_rates(mcrs)
        if (len(common_vco_rates)) == 0:
            return False, None, None
        # Now we can be sure we have a valid number here
        lmk_vco = int(common_vco_rates[0])
        # Calculate which LMK output rate is required to serve both DBs (least common multiple):
        required_rate = lcm(mcrs[0], mcrs[1])
        lmk_od = None
        if mcrs[0] == mcrs[1] or all(
            int(required_rate / mcr) in X4xxRfdcCtrl.RFDC_RESAMPLER for mcr in mcrs
        ):
            lmk_od = int(lmk_vco / required_rate)
            # Let the RFDC decimation do most work:
            for dec in X4xxRfdcCtrl.RFDC_RESAMPLER:
                if (
                    lmk_od % dec == 0
                    and X4xxRfdcCtrl.CONV_RATE_MIN
                    <= lmk_vco / (lmk_od / dec)
                    <= X4xxRfdcCtrl.CONV_RATE_MAX
                ):
                    lmk_od = int(lmk_od / dec)
                    # Keep using bypass
                    use_pll_bypass = True
                    break
                # Bypass not possible
                use_pll_bypass = False
        return use_pll_bypass, lmk_vco, lmk_od

    def _reduce_rfdc_frequency_list(self, resampling_factor):
        """Select RFDC frequencies compatible with given resampling factor.

        Filters the RFDC frequency list to only contain frequencies that can
        be supported with a valid SYSREF frequency.
        """
        return [
            rate
            for rate in RFDC_PLL_CONFIGS.keys()
            if len(self._find_sysref_matches(rate / resampling_factor)) > 0
        ]

    def _find_sysref_matches(self, mcr):
        """Search feasable SYSREF frequencies.

        Find all SYSREF frequencies that are compatible with the given
        converter rate and resampling factor.
        """
        return [
            sysref_rate
            for sysref_rate in self._valid_sysref_freqs
            if mcr / self._spc % sysref_rate == 0
        ]

    def _match_mcrs(self, desired_mcr, desired_conv_rate=0):
        """Coerces MCR and converter rate.

        With a given master clock rate and converter rate, check the value against
        what the RFDC is able to do and return the rounded values.
        """
        # Calculate desired converter rate: If a converter rate was passed, check if
        # we can decimate from converter rate to master clock rate and if this converter rate
        # is within the valid range. If this is impossible, pick the converter rate automatically.
        div = desired_conv_rate / desired_mcr
        coerced_mcr = desired_mcr
        if (
            div not in X4xxRfdcCtrl.RFDC_RESAMPLER
            or not X4xxRfdcCtrl.CONV_RATE_MIN <= desired_conv_rate <= X4xxRfdcCtrl.CONV_RATE_MAX
            or desired_mcr not in MCR_LMK_VCO
        ):
            coerced_mcr = min(sorted(MCR_LMK_VCO), key=lambda x: abs(x - desired_mcr))
            for div in sorted(X4xxRfdcCtrl.RFDC_RESAMPLER, reverse=True):
                if coerced_mcr * div <= X4xxRfdcCtrl.CONV_RATE_MAX:
                    break
        return coerced_mcr, coerced_mcr * div

    def _get_common_lmk_vco_rates(self, mcrs):
        """Returns a list of LMK VCO rates that can be used with both master clock rates."""
        vco_rates = [set(MCR_LMK_VCO[mcr]) for mcr in mcrs]
        return list(vco_rates[0].intersection(vco_rates[1]))

    def _get_common_spll_out_freqs(self, conv_rates, mcrs):
        """Calculates possible common LMK output frequencies to achieve the two converter rates."""
        # First get the required VCO rates for both DBs
        vco_rates = self._get_common_lmk_vco_rates(mcrs)
        # If we don't have common values, there is no common spll_out_freq, so we return early
        if len(vco_rates) == 0:
            return []

        # Get a list of all possible spll out frequencies for these converter rates
        spll_out_freqs = list(
            set.intersection(*[set(v) for k, v in RFDC_PLL_CONFIGS.items() if k in conv_rates])
        )
        # Then return them filtered for the VCO rates that are allowed for the MCRs
        return list(filter(lambda x: vco_rates[0] % x == 0, spll_out_freqs))

    def _get_max_mcr(self):
        """Returns the maximum sample rate based on the DSP BW reported by the FPGA."""
        # _dsp_bw is in MHz, multiplied by 1.28 for the sample rate necessary to capture this, e.g.
        # 1.6 GS/s (FPGA) * 1.28 = 2.048 GS/s (max sample rate)
        # 400 MS/s * 1.28 = 512 GS/s
        dsp_bw = self._dsp_bw
        if dsp_bw is None:
            # Play it safe and assume the lowest known dsp_bw
            dsp_bw = min(self.bandwidth_to_default_mcr.keys())
        return dsp_bw * 1e6 * 1.28

    def _get_min_mcr(self):
        """Returns the minimum sample rate allowed in X440."""
        return (
            X4xxRfdcCtrl.CONV_RATE_MIN / max(X4xxRfdcCtrl.RFDC_RESAMPLER) / self._extra_resampling
        )

    def _get_mmcm_rates(self, rfdc_rate, lmk_vco, sysref_rate):
        """Find MMCM config."""
        min_common_rfdc_rate = lcm(rfdc_rate[0], rfdc_rate[1])
        # Find the MMCM VCO rate that fits into the valid range and can serve both MCRs
        # Ceil operation for finding the first multiple of min_common_rfdc_rate that fits into range
        mmcm_vco_fit_factor = int(math.ceil(X4xxRfdcCtrl.MMCM_VCO_MIN / min_common_rfdc_rate))
        # Lowest potential MMCM VCO Rate
        min_mmcm_vco_rate = int(min_common_rfdc_rate * mmcm_vco_fit_factor)
        mmcm_cfg = {}
        for mmcm_vco_rate in range(
            min_mmcm_vco_rate, int(X4xxRfdcCtrl.MMCM_VCO_MAX + 1), min_common_rfdc_rate
        ):
            # Find the feedback divider and the PRC divider
            # MMCM input divider is always 1. Let's find the greatest possible numbers
            # both for the LMK PRC output divider and the feedback divider.
            # First get the GCD of LMK VCO and MMCM VCO (that's a possible MMCM input freq
            # but maybe too large)
            mmcm_input_max = math.gcd(int(lmk_vco), mmcm_vco_rate)
            mmcm_fb = int(mmcm_vco_rate / mmcm_input_max)
            mmcm_input = False
            # Might be that the calculated MMCM_FB doesn't fit the valid range, then try the next
            # possible MMCM VCO Rate immediately, otherwise check if a valid feedback divider can
            # be found that plays together nicely with the LMK VCO and the PRC divider.
            # Divide this mmcm_input until it fits into the MMCM input range:
            for div in range(mmcm_fb, X4xxRfdcCtrl.MMCM_FB_MAX + 1, mmcm_fb):
                mmcm_input = mmcm_vco_rate / div
                # MMCM input frequency needs to be in range and...
                if (
                    X4xxRfdcCtrl.MMCM_INPUT_MIN <= mmcm_input <= X4xxRfdcCtrl.MMCM_INPUT_MAX
                    and
                    # MMCM input frequency should be an integer value and...
                    not mmcm_input % 1
                    and
                    # the LMK PRC output divider should be an integer and...
                    not lmk_vco / mmcm_input % 1
                    and
                    # rc_div*2 is the fastest clock with smallest divider and that
                    # needs to be an integer, too:
                    not (mmcm_vco_rate / rfdc_rate[0] / 2) % 1
                    and not (mmcm_vco_rate / rfdc_rate[1] / 2) % 1
                    and
                    # RFDC rate must be a multiple of the PRC(==mmcm_input)
                    all([(rate % mmcm_input) == 0 for rate in rfdc_rate])
                    and
                    # PRC must be an integer multiple of SYSREF
                    mmcm_input % sysref_rate == 0
                ):
                    if not mmcm_cfg.get(mmcm_input) or mmcm_cfg.get(mmcm_input) > mmcm_vco_rate:
                        mmcm_cfg.update({mmcm_input: mmcm_vco_rate})
                mmcm_input = False
            if mmcm_input:
                break
        if len(mmcm_cfg.keys()) == 0:
            error_msg = (
                "Unable to find a valid MMCM configuration for Master Clock Rate(s)"
                ' requested. Refer to "About Sampling Rates and Master Clock Rates'
                ' for the USRP X440" in Knowledge Base for more information on'
                " supported rates."
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        return max(mmcm_cfg), mmcm_cfg[max(mmcm_cfg)]

    def get_intermediate_clk_settings(self, ref_clk_freq, old_mcrs, new_mcrs):
        """Returns an intermediate clock settings object.

        An  intermediate object is necessary if going from the old to
        the new master clock rates would fail otherwise.
        """
        # TODO we can be smarter here -- not all transitions require this.
        if tuple(old_mcrs) != tuple(new_mcrs):
            return self.get_config(ref_clk_freq, [250e6, 250e6])
        return None

    def coerce_mcr(self, master_clock_rates):
        """Validate that the requested master clock rate is valid.

        Will throw a ValueError if not.

        Does not modify the state of this policy.
        """
        self._read_converter_rates()
        # We might get the MCR as tuple, so convert it to be on the safe side.
        master_clock_rates = list(master_clock_rates)
        if len(master_clock_rates) < self.get_num_rates():
            master_clock_rates = [master_clock_rates[0]] * self.get_num_rates()

        # Check if MCR meets BW
        for mcr in master_clock_rates:
            if not self._get_min_mcr() <= mcr <= self._get_max_mcr():
                raise ValueError(
                    f"Invalid master clock rate {mcr/1e6} MHz for current FPGA image "
                    f"with bandwidth {self._dsp_bw} MHz!"
                )

        # Check if we can generate these MCRs with the RFDC at all
        mcrs = []
        conv_rates = []
        for idx, mcr in enumerate(master_clock_rates):
            conv_rate = 0 if self.conv_rates is None else self.conv_rates[idx]
            mcr, cvr = self._match_mcrs(mcr, conv_rate)
            mcrs.append(mcr)
            conv_rates.append(cvr)
        # Find out if we can generate the two rates with the same LMK output frequency
        common_out = self._get_common_spll_out_freqs(conv_rates, mcrs)
        # Use the first MCR for both if we cannot combine them.
        if len(common_out) == 0:
            mcrs[1] = mcrs[0]
            # Ensure we're falling back to converter rate 0, too
            conv_rates[1] = conv_rates[0]

        # With these values check if we can do this in MMCM
        rfdc_rate = list(map(lambda x: int(x / (self._spc * self._extra_resampling)), mcrs))
        min_mmcm_vco_rate = lcm(rfdc_rate[0], rfdc_rate[1])
        # Inform user if we don't have an exact match
        if min_mmcm_vco_rate > X4xxRfdcCtrl.MMCM_VCO_MAX:
            mcrs[1] = mcrs[0]
            # Ensure we're falling back to converter rate 0, too
            conv_rates[1] = conv_rates[0]

        if mcrs != master_clock_rates:
            self.log.warning(
                f"Unable to use desired master clock rate(s), using "
                f"{mcrs[0]/1e6} MHz for DB0 and {mcrs[1]/1e6} MHz for DB1."
            )
        if self.conv_rates is not None and self.conv_rates != conv_rates:
            self.log.warning(
                f"Unable to use desired converter rate(s), using "
                f"{conv_rates[0]/1e6} MSps for DB0 "
                f"and {conv_rates[1]/1e6} MSps for DB1. Converter rate needs "
                f"to be a {X4xxRfdcCtrl.RFDC_RESAMPLER} multiple of the master "
                f"clock rates {mcrs[0]/1e6} MHz and {mcrs[1]/1e6} MHz "
                f"but additional clock constraints may limit this."
            )
        self.conv_rates = conv_rates
        return mcrs

    def get_config(self, ref_clock_freq, master_clock_rates):
        """Returns a valid configuration based on the master clock rate.

        It uses the configuration where the RFDC_CLOCK/SPC is the closest to the MCR
        This method is called after coerce_mcr() has run and - if necessary -
        rounded the MCR values, so will skip the checks here to save some time.
        """
        if len(master_clock_rates) != self.get_num_rates():
            master_clock_rates = [master_clock_rates[0]] * self.get_num_rates()

        # Get us the rounded mcr with fitting converter rates
        mcrs = []
        conv_rates = []
        for idx, mcr in enumerate(master_clock_rates):
            conv_rate = 0 if self.conv_rates is None else self.conv_rates[idx]
            mcr, cvr = self._match_mcrs(mcr, conv_rate)
            mcrs.append(mcr)
            conv_rates.append(cvr)
        # Check if we can bypass the RFDC PLL (lower phase noise)
        bypass, lmk_vco, lmk_od = self._match_bypass_freqs(mcrs)
        bypass_conv_rate = 0 if lmk_od is None else lmk_vco / lmk_od

        # Only bypass if this exact converter rate is desired
        if bypass and bypass_conv_rate in conv_rates:
            output_freq = lmk_vco / lmk_od
            self.log.info("Bypassing RFDC PLL")
            conv_rate = lmk_vco / lmk_od
            conv_rates = [conv_rate, conv_rate]
        else:
            # Get the common LMK output freq for both MCRs
            common_out = self._get_common_spll_out_freqs(conv_rates, mcrs)
            assert len(common_out) > 0
            self.log.info("Using RFDC PLL")
            # Use the maximum value
            output_freq = max(common_out)
            # Find out how to configure the LMK for this output freq:
            lmk_vco = self._get_common_lmk_vco_rates(mcrs)[0]
            lmk_od = int(lmk_vco / output_freq)
        self.conv_rates = conv_rates
        spll1_vco = 100e6 if lmk_vco % 100e6 == 0 else 122.88e6
        pll2_n = lmk_vco / spll1_vco
        pll2_prescaler = [x for x in LMK04832.PLL2_PRESCALER if not pll2_n % x]
        pll2_n_div = int(pll2_n / pll2_prescaler[0])

        # Looking for the correct sysref config needed for both MCRs
        sysref_config = next(
            sysref_setting
            for sysref_setting in LMK04832X4xx.SYSREF_CONFIG[spll1_vco]
            if all(sysref_setting["SYSREF_FREQ"] in self._find_sysref_matches(mcr) for mcr in mcrs)
        )

        # The following asserts are to explicitly check for SYSREF requirements
        # as per pg269, Ch. 4, Section "SYSREF Signal Requirements"
        assert sysref_config["SYSREF_FREQ"] < 10e6, "SysRef frequency exceeds limit of 10 MHz"
        assert all(
            conv_rate % sysref_config["SYSREF_FREQ"] == 0.0 for conv_rate in conv_rates
        ), "Converter Rate is not a multiple of the SysRef freq"

        rfdc_rate = list(map(lambda x: int(x / (self._spc * self._extra_resampling)), mcrs))
        mmcm_input, mmcm_vco_rate = self._get_mmcm_rates(
            rfdc_rate, lmk_vco, sysref_config["SYSREF_FREQ"]
        )
        prc_rate = mmcm_input

        if not mmcm_input or mmcm_input > lmk_vco:
            error_msg = (
                f"Unable to find a valid MMCM configuration for Master Clock Rate(s) "
                f"{mcrs[0]/1e6} MHz and {mcrs[1]/1e6} MHz. Please choose different "
                f"Master Clock Rate(s)."
            )
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        prc_div = int(lmk_vco / mmcm_input)
        assert prc_div in LMK04832X4xx.PRC_OUT_DIVIDERS, "Invalid LMK PRC output divider."
        mmcm_fb = int(mmcm_vco_rate / mmcm_input)
        assert (
            X4xxRfdcCtrl.MMCM_FB_MIN <= mmcm_fb <= X4xxRfdcCtrl.MMCM_FB_MAX
        ), "Invalid MMCM Feedback Divider."
        r0_clk = int(mmcm_vco_rate / rfdc_rate[0])
        assert (
            X4xxRfdcCtrl.MMCM_OD_MIN <= r0_clk <= X4xxRfdcCtrl.MMCM_OD_MAX
        ), "Invalid MMCM output divider for radio 0 clock."
        r1_clk = int(mmcm_vco_rate / rfdc_rate[1])
        assert (
            X4xxRfdcCtrl.MMCM_OD_MIN <= r1_clk <= X4xxRfdcCtrl.MMCM_OD_MAX
        ), "Invalid MMCM output divider for radio 1 clock."
        r0_clk2x = int(r0_clk / 2)
        assert (
            X4xxRfdcCtrl.MMCM_OD_MIN <= r0_clk2x <= X4xxRfdcCtrl.MMCM_OD_MAX
        ), "Invalid MMCM output divider for radio 0 clock 2x."
        r1_clk2x = int(r1_clk / 2)
        assert (
            X4xxRfdcCtrl.MMCM_OD_MIN <= r1_clk2x <= X4xxRfdcCtrl.MMCM_OD_MAX
        ), "Invalid MMCM output divider for radio 1 clock 2x."
        prc_out_div = int(mmcm_vco_rate / prc_rate)
        assert (
            X4xxRfdcCtrl.MMCM_OD_MIN <= prc_out_div <= X4xxRfdcCtrl.MMCM_OD_MAX
        ), "Invalid MMCM output divider for PRC."

        spll_cfg = {
            "ref_freq": ref_clock_freq,
            "output_freq": output_freq,
            "output_divider": lmk_od,
            "prc_divider": prc_div,
            "vcxo_freq": Spll1Vco.VCO100MHz if spll1_vco == 100e6 else Spll1Vco.VCO122_88MHz,
            "sysref_div": int(lmk_vco / sysref_config["SYSREF_FREQ"]),
            "sysref_delay": int(lmk_vco / prc_rate / 2),
            "clkin0_r_div": int(ref_clock_freq / sysref_config["PDF"]),
            "pll1_n_div": int(sysref_config["SYSREF_FREQ"] / sysref_config["PDF"]),
            "pll2_prescaler": int(pll2_prescaler[0]),
            "pll2_n_cal_div": pll2_n_div,
            "pll2_n_div": pll2_n_div,
            "prc_to_db": False,  # X440 does not have CPLD on DB, so turn off toggling PRC signal
        }
        rfdc_cfg0 = {"conv_rate": conv_rates[0], "resampling": int(conv_rates[0] / mcrs[0])}
        rfdc_cfg1 = {"conv_rate": conv_rates[1], "resampling": int(conv_rates[1] / mcrs[1])}
        clk_config = {
            "mmcm_use_defaults": False,
            "mmcm_feedback_divider": mmcm_fb,
            "mmcm_input_divider": 1,
            "mmcm_output_div_map": {
                "r0_clk": r0_clk,
                "r1_clk": r1_clk,
                "r0_clk_2x": r0_clk2x,
                "r1_clk_2x": r1_clk2x,
                "prc": prc_out_div,
            },
            "rfdc_configs": [RfdcConfig(**rfdc_cfg0), RfdcConfig(**rfdc_cfg1)],
            "spll_config": SpllConfig(**spll_cfg),
        }

        return X4xxClockConfig(**clk_config)

    def should_reboot_on_reconfiguration(self):
        """X4xx requires MPM restart after clock reconfiguration."""
        return True


def get_clock_policy(mboard_info, dboard_infos, args, log):
    """Return a clocking policy object based on the available hardware."""
    if dboard_infos[0]["pid"] in ZBX.pids:
        return X410ClockPolicy(mboard_info, dboard_infos, args, log)
    if dboard_infos[0]["pid"] in FBX.pids:
        return X440ClockPolicy(mboard_info, dboard_infos, args, log)
    raise RuntimeError("Could not resolve clock policy.")
