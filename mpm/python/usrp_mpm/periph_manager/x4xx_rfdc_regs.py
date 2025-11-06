#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4xx RFDC register control
"""

import time

from usrp_mpm.chips.ic_reg_maps.x4xx_rfdc_regmap import x4xx_rfdc_regmap_t
from usrp_mpm.mpmutils import poll_with_timeout
from usrp_mpm.periph_manager.x4xx_clock_lookup import (
    MMCM_FILTERGROUP_LOOKUP,
    MMCM_LOCKGROUP_LOOKUP,
)
from usrp_mpm.sys_utils.uio import UIO

# Number of ADCs and DACs in the RFDC.
NUM_CONVERTERS = 16


class RfdcRegsControl:
    """
    Control the FPGA RFDC registers external to the XRFdc API
    """

    mmcm_clock_map = {
        "prc": 0,
        "data_clk": 1,
        "data_clk_2x": 3,
        "rfdc_clk": 2,
        "rfdc_clk_x2": 4,
        # These are synonymous to rfdc_clk
        "r0_clk": 2,
        "r0_clk_2x": 4,
        "r1_clk": 5,
        "r1_clk_2x": 6,
    }

    def __init__(self, label, log):
        self.log = log.getChild("RfdcRegs")
        self.regs = UIO(label=label, read_only=False)
        self._regs = x4xx_rfdc_regmap_t()
        # All registers that we need to know the value of from the beginning we
        # need to read out here. Note that we can't yet read all registers when
        # this class is instantiated, as the RFDC itself is still being held in
        # reset. See also reset().
        self._update_reg("CAL_ENABLE_DB0_CHAN0")
        self._update_reg("FABRIC_DSP_INFO_BW")
        self._update_reg("RFDC_INFO_DB0_XTRA_RESAMP")
        for conv in range(NUM_CONVERTERS):
            self._update_reg("RFDC_INFO_BLOCK_MODE", conv)
        self._regs.save_state()
        # From now on, we can use _commit() to write registers instead of going
        # through peek/poke.
        self._converter_chains_in_reset = True

    def reset(self):
        """
        Resets the state of the register object. Call this after pulling the
        RFDC out of reset.
        """
        self._update_reg("CAL_ENABLE_DB0_CHAN0")
        self._update_reg("IQ_SWAP_DAC_DB0_CHAN0")
        self._update_reg("IQ_SWAP_DAC_DB1_CHAN0")
        self._regs.save_state()

    def get_num_rx_channels(self):
        """
        Returns the number of RX channels as defined in the RFDC register.
        """
        return [
            getattr(self._regs, f"FABRIC_DSP_INFO_DB0_RX_CNT"),
            getattr(self._regs, f"FABRIC_DSP_INFO_DB1_RX_CNT"),
        ]

    def get_num_tx_channels(self, board=0):
        """
        Returns the number of TX channels as defined in the RFDC register.
        """
        return [
            getattr(self._regs, f"FABRIC_DSP_INFO_DB0_TX_CNT"),
            getattr(self._regs, f"FABRIC_DSP_INFO_DB1_TX_CNT"),
        ]

    def get_converter_mapping(self):
        """
        Returns two tuples: (adc_mapping, dac_mapping).

        Every tuple consists of two sub-tuples: (db0_map, db1_map).

        Every map contains yet another tuple per channel: (tile, block).

        For example, adc_mapping might be of value:
        (
            (
                (0, 1), (0, 0)
            ), (
                (2, 1), (2, 0)
            )
        )

        This means there are 2 dboards. On the first dboard, we have 2 channels,
        and channel 0 uses tile 0, block 1.
        """

        # define helper function to extract content from device memory
        def get_tuple(db, channel, is_adc):
            for i in range(NUM_CONVERTERS):
                if (
                    (
                        self._regs.RFDC_INFO_BLOCK_MODE[i]
                        == self._regs.RFDC_INFO_BLOCK_MODE_t.RFDC_INFO_BLOCK_MODE_ENABLED
                    )
                    and (self._regs.RFDC_INFO_DB[i] == db)
                    and (self._regs.RFDC_INFO_CHANNEL[i] == channel)
                    and (self._regs.RFDC_INFO_IS_ADC[i] == is_adc)
                ):
                    return tuple((self._regs.RFDC_INFO_TILE[i], self._regs.RFDC_INFO_BLOCK[i]))
            else:
                raise ValueError(f"Could not find mapping for {db}, {channel}, {is_adc}")

        adc_mapping = tuple(
            tuple(get_tuple(db, chan, True) for chan in range(num_channels))
            for db, num_channels in enumerate(self.get_num_rx_channels())
        )
        dac_mapping = tuple(
            tuple(get_tuple(db, chan, False) for chan in range(num_channels))
            for db, num_channels in enumerate(self.get_num_tx_channels())
        )
        return adc_mapping, dac_mapping

    def get_rfdc_info(self, db_idx):
        """
        Return a dictionary with information about the RFDC configuration.
        """
        assert db_idx in (0, 1)
        return {
            "num_rx_chans": getattr(self._regs, f"FABRIC_DSP_INFO_DB{db_idx}_RX_CNT"),
            "num_tx_chans": getattr(self._regs, f"FABRIC_DSP_INFO_DB{db_idx}_TX_CNT"),
            "bw": self._regs.FABRIC_DSP_INFO_BW,
            "extra_resampling": getattr(self._regs, f"RFDC_INFO_DB{db_idx}_XTRA_RESAMP"),
            "spc_rx": 2 ** getattr(self._regs, f"RFDC_INFO_DB{db_idx}_SPC_RX"),
            "spc_tx": 2 ** getattr(self._regs, f"RFDC_INFO_DB{db_idx}_SPC_TX"),
        }

    def get_threshold_status(self, db_idx, channel, threshold_idx):
        """
        Retrieves the status bit for the given threshold block
        """
        adc_mapping, _ = self.get_converter_mapping()
        assert len(adc_mapping) > 0
        assert 0 <= db_idx < len(adc_mapping)
        assert 0 <= channel < len(adc_mapping[0])
        assert threshold_idx in [0, 1]

        adc, block = adc_mapping[db_idx][channel]
        reg_name = f"THRESHOLD_ADC{adc}_BLOCK{block}_IDX{threshold_idx}"
        self._update_reg(reg_name)
        status = getattr(self._regs, reg_name)
        return bool(status)

    def set_cal_data(self, i, q):
        """
        Set I and Q data to be used for cal tones. I and Q are treated
        as signed values (2-complement).
        """
        assert 0 <= i < 2**16
        assert 0 <= q < 2**16
        self._regs.CAL_DATA_I = i
        self._regs.CAL_DATA_Q = q
        self._commit()

    def set_cal_enable(self, db_idx, channel, enable):
        """
        Enable the cal tone for the specified TX channel. The cal tone is
        muxed into the TX signal path.
        """
        info = self.get_rfdc_info(db_idx)
        assert 0 <= channel < info["num_tx_chans"]
        assert enable in [False, True]
        setattr(self._regs, f"CAL_ENABLE_DB{db_idx}_CHAN{channel}", int(enable))
        self._commit()

    def enable_iq_swap(self, enable, db_id, chan_idx, is_dac):
        assert db_id in (0, 1)
        num_channels = (
            self.get_num_tx_channels()[db_id] if is_dac else self.get_num_rx_channels()[db_id]
        )
        assert chan_idx in range(num_channels)
        assert int(is_dac) in (0, 1)
        addr_name = f'IQ_SWAP_{"DAC" if is_dac else "ADC"}_DB{db_id}_CHAN{chan_idx}'
        enum_name = addr_name + "_t"
        setattr(self._regs, addr_name, getattr(self._regs, enum_name)(enable))
        self._commit()

    ###########################################################################
    # MMCM control
    ###########################################################################
    def set_reset_mmcm(self, reset=True):
        """
        Put MMCM into reset, or take it out of reset
        """
        self._regs.MMCM_RESET = (
            self._regs.MMCM_RESET_t.MMCM_RESET_ENABLE
            if reset
            else self._regs.MMCM_RESET_t.MMCM_RESET_DISABLE
        )
        self._commit()

    def reconfigure_mmcm(self, use_regs=True):
        """
        Reconfigure the MMCM, either through the DRP registers, or from the
        hard-coded defaults.

        We assume we're using the MMCM through the clock configuration wizard,
        for documentation on this register and this procedure, cf. pg065, Table
        2-2, "Clock Configuration Register".
        """
        # Commit pending configuration changes
        self._commit()
        self._regs.MMCM_LOAD_SEN = 1
        self._regs.MMCM_SADDR = int(use_regs)
        self.log.trace("MMCM DRP initiated.")
        # There seems to be issues when committing this cached value, so we
        # poke directly instead.
        self._poke(
            self._regs.get_addr("MMCM_LOAD_SEN"),
            self._regs.get_reg(self._regs.get_addr("MMCM_LOAD_SEN")),
        )

        self.log.trace("MMCM Configuration applied, now waiting for MMCM to lock...")
        self.wait_for_mmcm_drp_done()
        self.clear_data_clk_unlocked()
        self._commit()

    def clear_data_clk_unlocked(self):
        """
        Currently required when checking the MMCM lock.
        FIXME: Quick & dirty workaround for CLEAR_DATA_CLK_UNLOCKED bit. Needs to be
        removed when this is resolved in digital.
        """
        time.sleep(1)
        self._regs.CLEAR_DATA_CLK_UNLOCKED = self._regs.CLEAR_DATA_CLK_UNLOCKED_t(1)
        self._commit()
        self._regs.CLEAR_DATA_CLK_UNLOCKED = self._regs.CLEAR_DATA_CLK_UNLOCKED_t(0)
        self._commit()

    def wait_for_mmcm_locked(self, timeout=0.001):
        """
        Wait for MMCM to come to a stable locked state.
        The datasheet specifies a 100us max lock time
        """
        self.wait_for_mmcm_drp_done()

        def check_lock():
            self._update_reg("MMCM_LOCKED")
            if self._regs.MMCM_LOCKED:
                self.log.trace("RF MMCM lock detected.")
                return True
            return False

        poll_sleep_ms = 0.2
        if not poll_with_timeout(check_lock, timeout * 1000, poll_sleep_ms):
            self.log.error("MMCM failed to lock in the expected time.")
            raise RuntimeError("MMCM failed to lock within the expected time.")
        self.clear_data_clk_unlocked()

    def wait_for_mmcm_drp_done(self, timeout=0.001):
        """
        Poll the MMCM_LOAD_SEN bit. If we reconfigured the MMCM, it could be
        high and we need to wait for it to be low for the reconfig to be
        successful.
        If we didn't reconfigure the MMCM, this will always be low.
        """
        if self._regs.MMCM_LOAD_SEN:
            # If this ^^^ is true, then we previously requested a dynamic reconfig
            # of the MMCM, so we'll check this bit is also de-asserted, which it
            # should be according to pg065.
            poll_sleep_ms = 0.2

            def check_reconfig_done():
                self._update_reg("MMCM_LOAD_SEN")
                return not self._regs.MMCM_LOAD_SEN

            if not poll_with_timeout(check_reconfig_done, timeout * 1000, poll_sleep_ms):
                self.log.error("MMCM failed to confirm DRP in the expected time.")
                raise RuntimeError("MMCM failed to confirm DRP in the expected time.")
            # We need to tell the reg cache that the value changed back:
            self._regs.save_state()
            self.log.trace("MMCM DRP successfully completed.")

    def update_mmcm_regs(self):
        """
        Update the saved state of the MMCM registers from the hardware
        """
        self.log.debug("Resetting mmcm registers")
        # MMCM register range from 0x300 up to 0x35C
        mmcm_regs = range(0x300, 0x35D, 0x4)

        for reg in mmcm_regs:
            reg_val = self._peek(reg)
            self._regs.set_reg(reg, reg_val)
        self._regs.save_state()

    def set_mmcm_div(self, div_val):
        """
        Set MMCM input divider.

        Does not commit register values!
        """
        assert isinstance(div_val, int) and 1 <= div_val < 127
        div_val_by_2 = div_val // 2
        if div_val == 1:
            self._regs.MMCM_DIV_LO_TIME = 0
            self._regs.MMCM_DIV_HI_TIME = 0
            self._regs.MMCM_DIV_NO_COUNT = 1
            self._regs.MMCM_DIV_EDGE = 0
        elif div_val % 2:  # Odd divisions
            self._regs.MMCM_DIV_LO_TIME = div_val_by_2 + 1
            self._regs.MMCM_DIV_HI_TIME = div_val_by_2
            self._regs.MMCM_DIV_NO_COUNT = 0
            self._regs.MMCM_DIV_EDGE = 1
        else:  # Even divisions
            self._regs.MMCM_DIV_LO_TIME = div_val_by_2
            self._regs.MMCM_DIV_HI_TIME = div_val_by_2
            self._regs.MMCM_DIV_NO_COUNT = 0
            self._regs.MMCM_DIV_EDGE = 0

    def set_mmcm_output_div(self, div_val, clock_name):
        """
        Set the output divider of a given clock

        clock_name is either a string name of a clock ('rfdc_clk', 'data_clk'
        and so on) or an integer, in which case it's the *zero-based* index of
        the clock (0, 1, 2, etc.). Note that the Clock Wizard GUI uses a 1-based
        index, but pg065 uses a 0-based index to describe the registers. Between
        these two, we choose the sane option.

        Does not commit register values!
        """
        assert clock_name in self.mmcm_clock_map or (
            isinstance(clock_name, int) and 0 <= clock_name <= 6
        )
        clock_idx = (
            clock_name if isinstance(clock_name, int) else self.mmcm_clock_map.get(clock_name)
        )
        div_val_int = int(div_val)
        assert 1 <= div_val_int <= 128
        div_val_int_by2 = div_val_int // 2
        div_val_frac = div_val - div_val_int
        assert div_val_frac == 0.0 or (clock_idx == 0 and ((div_val_frac / 0.125) % 1 == 0))
        if div_val_int == 1:
            setattr(self._regs, f"CLKOUT{clock_idx}_LO_TIME", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_HI_TIME", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_NO_COUNT", 1)
            setattr(self._regs, f"CLKOUT{clock_idx}_EDGE", 0)
        elif div_val_int == 128:
            # The 128 case is undocumented in pg065 and xapp888, but the MMCM
            # does have a documented max division of 128, and a division value
            # of 0 is invalid so it can be used for this case. This was confirmed
            # by setting the division to 128 in the clock wizard GUI and reading
            # out the resulting register value.
            setattr(self._regs, f"CLKOUT{clock_idx}_LO_TIME", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_HI_TIME", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_NO_COUNT", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_EDGE", 0)
        elif div_val_int % 2:  # Odd divisions
            setattr(self._regs, f"CLKOUT{clock_idx}_LO_TIME", div_val_int_by2 + 1)
            setattr(self._regs, f"CLKOUT{clock_idx}_HI_TIME", div_val_int_by2)
            setattr(self._regs, f"CLKOUT{clock_idx}_NO_COUNT", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_EDGE", 1)
        else:  # Even divisions
            setattr(self._regs, f"CLKOUT{clock_idx}_LO_TIME", div_val_int_by2)
            setattr(self._regs, f"CLKOUT{clock_idx}_HI_TIME", div_val_int_by2)
            setattr(self._regs, f"CLKOUT{clock_idx}_NO_COUNT", 0)
            setattr(self._regs, f"CLKOUT{clock_idx}_EDGE", 0)
        setattr(self._regs, f"CLKOUT{clock_idx}_MX", 0)
        if clock_idx == 0:
            div_val_frac_code = int(div_val_frac / 0.125)
            self._regs.CLKOUT0_FRAC_EN = int(div_val_frac != 0)
            self._regs.CLKOUT0_FRAC = div_val_frac_code
            # No idea what these need to be
            self._regs.CLKOUT0_PHASE_MUX_F = 0
            self._regs.CLKOUT0_FRAC_WF_R = 0
            self._regs.CLKOUT0_FRAC_WF_F = 0

        self._commit()

    def get_mmcm_output_div(self, clock_name):
        """
        Get the output divider of a given clock

        For clock_name, see set_mmcm_output_div().
        """
        assert clock_name in self.mmcm_clock_map or (
            isinstance(clock_name, int) and 0 <= clock_name <= 6
        )
        clock_idx = (
            clock_name if isinstance(clock_name, int) else self.mmcm_clock_map.get(clock_name)
        )
        self._update_reg(f"CLKOUT{clock_idx}_LO_TIME")
        self._update_reg(f"CLKOUT{clock_idx}_NO_COUNT")
        lo_time = getattr(self._regs, f"CLKOUT{clock_idx}_LO_TIME")
        hi_time = getattr(self._regs, f"CLKOUT{clock_idx}_HI_TIME")
        no_count = getattr(self._regs, f"CLKOUT{clock_idx}_NO_COUNT")
        div_val = 1 if no_count else lo_time + hi_time
        if div_val == 0:
            div_val = 128
        if clock_idx == 0 and self._regs.CLKOUT0_FRAC_EN:
            div_val += 0.125 * self._regs.CLKOUT0_FRAC
        return div_val

    def set_mmcm_fb_div(self, div_val):
        """
        Set the feedback divider of a given clock

        Does not commit register values!
        """
        div_val_int = int(div_val)
        div_val_int_by2 = div_val_int // 2
        div_val_frac = div_val - div_val_int
        assert 1 <= div_val_int <= 128
        assert (div_val_frac / 0.125) % 1 == 0
        if div_val_int == 1:
            self._regs.CLKFBOUT_LO_TIME = 0
            self._regs.CLKFBOUT_HI_TIME = 0
            self._regs.CLKFBOUT_NO_COUNT = 1
            self._regs.CLKFBOUT_EDGE = 0
        elif div_val_int % 2:  # Odd divisions
            self._regs.CLKFBOUT_LO_TIME = div_val_int_by2 + 1
            self._regs.CLKFBOUT_HI_TIME = div_val_int_by2
            self._regs.CLKFBOUT_NO_COUNT = 0
            self._regs.CLKFBOUT_EDGE = 1
        else:  # Even divisions
            self._regs.CLKFBOUT_LO_TIME = div_val_int_by2
            self._regs.CLKFBOUT_HI_TIME = div_val_int_by2
            self._regs.CLKFBOUT_NO_COUNT = 0
            self._regs.CLKFBOUT_EDGE = 0
        self._regs.CLKFBOUT_MX = 0
        div_val_frac_code = int(div_val_frac / 0.125)
        self._regs.CLKFBOUT_FRAC_EN = int(div_val_frac != 0)
        self._regs.CLKFBOUT_FRAC = div_val_frac_code
        # No idea what these need to be
        self._regs.CLKFBOUT_PHASE_MUX_F = 0
        self._regs.CLKFBOUT_FRAC_WF_R = 0
        self._regs.CLKFBOUT_FRAC_WF_F = 0
        self._commit()

        # Fetch the register values from LUTs.
        # LUTs' first index corresponds to FB_DIV = 1
        lock_lookup = MMCM_LOCKGROUP_LOOKUP[div_val - 1]
        filt_lookup = MMCM_FILTERGROUP_LOOKUP[div_val - 1]

        # Lock lookup distribution (from XAPP888):
        # LOCKREG1[9:0]   = lock_lookup[29:20]
        # LOCKREG2[14:10] = lock_lookup[34:30]
        # LOCKREG2[9:0]   = lock_lookup[9:0]
        # LOCKREG3[14:10] = lock_lookup[39:35]
        # LOCKREG3[9:0]   = lock_lookup[19:10]
        self._regs.MMCM_LOCKREG1 = (lock_lookup & 0x003FF00000) >> 20

        self._regs.MMCM_LOCKREG2 = (lock_lookup & 0x00000003FF) >> 0 | (
            lock_lookup & 0x07C0000000
        ) >> (30 - 10)

        self._regs.MMCM_LOCKREG3 = (lock_lookup & 0x00000FFC00) >> 10 | (
            lock_lookup & 0xF800000000
        ) >> (35 - 10)

        # Filter lookup distribution (from XAPP888):
        # FILTREG1[15]    = filt_lookup[9]
        # FILTREG1[12:11] = filt_lookup[8:7]
        # FILTREG1[8]     = filt_lookup[6]
        # FILTREG2[15]    = filt_lookup[5]
        # FILTREG2[12:11] = filt_lookup[4:3]
        # FILTREG2[8:7]   = filt_lookup[2:1]
        # FILTREG2[4]     = filt_lookup[4]
        self._regs.MMCM_FILTREG1 = (
            (filt_lookup & 0x200) << (15 - 9)
            | (filt_lookup & 0x180) << (11 - 7)
            | (filt_lookup & 0x4) << (8 - 6)
        )

        self._regs.MMCM_FILTREG2 = (
            (filt_lookup & 0x20) << (15 - 5)
            | (filt_lookup & 0x18) << (11 - 3)
            | (filt_lookup & 0x6) << (7 - 1)
            | (filt_lookup & 0x1) << (4 - 0)
        )
        self._commit()

    def set_gated_clock_enables(self, value=True):
        """
        Controls the clock enable for data_clk, data_clk_2x and RFDC clocks

        This disables/enables the clock buffers on the MMCM.
        """
        self._regs.RF_PLL_ENABLE_DATA_CLK = self._regs.RF_PLL_ENABLE_DATA_CLK_t(int(value))
        self._regs.RF_PLL_ENABLE_DATA_CLK_x2 = self._regs.RF_PLL_ENABLE_DATA_CLK_x2_t(int(value))
        self._regs.RF_PLL_ENABLE_RF0_CLK = self._regs.RF_PLL_ENABLE_RF0_CLK_t(int(value))
        self._regs.RF_PLL_ENABLE_RF0_CLK_x2 = self._regs.RF_PLL_ENABLE_RF0_CLK_x2_t(int(value))
        self._regs.RF_PLL_ENABLE_RF1_CLK = self._regs.RF_PLL_ENABLE_RF1_CLK_t(int(value))
        self._regs.RF_PLL_ENABLE_RF1_CLK_x2 = self._regs.RF_PLL_ENABLE_RF1_CLK_x2_t(int(value))
        self._commit()

    def set_reset_adc_dac_chains(self, reset=True):
        """Resets or enables the ADC and DAC chain for the given dboard"""
        if not reset:
            # We actually don't keep the converters in reset, so this is a no-op.
            self._converter_chains_in_reset = False
            return
        if self._converter_chains_in_reset:
            self.log.debug("Converters are already in reset. " "The reset bit will NOT be toggled.")
            return

        # The actual reset procedure
        def _check_for_done(db_idx, xdc):
            """Query a specified done bit.  'xdc' must  be either 'ADC' or 'DAC."""
            assert xdc in ("ADC", "DAC")
            assert db_idx in (0, 1)
            reg_name = f"RFDC_DB{db_idx}_{xdc}_RESET_DONE"
            self._update_reg(reg_name)
            return getattr(self._regs, reg_name)

        reset_timeout = 5000  # ms
        poll_sleep = 1  # ms

        def poll_for_done(db_idx, xdc):
            """Shorthand for poll_with_timeout on the DONE bits"""
            return poll_with_timeout(
                lambda: _check_for_done(db_idx, xdc), reset_timeout, poll_sleep
            )

        # Reset the ADC chains
        self.log.trace("Resetting ADC chain")
        self._regs.RFDC_DB0_ADC_RESET = self._regs.RFDC_DB0_ADC_RESET_t.RFDC_DB0_ADC_RESET_ENABLE
        self._regs.RFDC_DB1_ADC_RESET = self._regs.RFDC_DB1_ADC_RESET_t.RFDC_DB1_ADC_RESET_ENABLE
        self._commit()
        for db_idx in (0, 1):
            if not poll_for_done(db_idx, "ADC"):
                self.log.error("Timeout while resetting ADC chains.")
                raise RuntimeError("Timeout while resetting ADC chains.")
        self._regs.RFDC_DB0_ADC_RESET = self._regs.RFDC_DB0_ADC_RESET_t.RFDC_DB0_ADC_RESET_DISABLE
        self._regs.RFDC_DB1_ADC_RESET = self._regs.RFDC_DB1_ADC_RESET_t.RFDC_DB1_ADC_RESET_DISABLE
        self._commit()
        # Reset the DAC chains
        self.log.trace("Resetting DAC chain")
        self._regs.RFDC_DB0_DAC_RESET = self._regs.RFDC_DB0_DAC_RESET_t.RFDC_DB0_DAC_RESET_ENABLE
        self._regs.RFDC_DB1_DAC_RESET = self._regs.RFDC_DB1_DAC_RESET_t.RFDC_DB1_DAC_RESET_ENABLE
        self._commit()
        for db_idx in (0, 1):
            if not poll_for_done(db_idx, "DAC"):
                self.log.error("Timeout while resetting DAC chains.")
                raise RuntimeError("Timeout while resetting DAC chains.")
        self._regs.RFDC_DB0_DAC_RESET = self._regs.RFDC_DB0_DAC_RESET_t.RFDC_DB0_DAC_RESET_DISABLE
        self._regs.RFDC_DB1_DAC_RESET = self._regs.RFDC_DB1_DAC_RESET_t.RFDC_DB1_DAC_RESET_DISABLE
        self._commit()
        self._converter_chains_in_reset = True

    def log_status(self):
        """
        Debugging API to dump the RFDC interface status.
        """
        self._update_reg("STATUS_RFDC_DB0_DAC_TREADY")
        # pylint: disable=f-string-without-interpolation
        # pylint: disable=invalid-name
        r = self._regs
        self.log.debug(f"Daughterboard 0")
        self.log.debug(f"  @RFDC")
        self.log.debug(f"    DAC(1:0) TREADY    : {r.STATUS_RFDC_DB0_DAC_TREADY:02b}")
        self.log.debug(f"    DAC(1:0) TVALID    : {r.STATUS_RFDC_DB0_DAC_TVALID:02b}")
        self.log.debug(f"    ADC(1:0) I TREADY  : {r.STATUS_RFDC_DB0_ADC_I_TREADY:02b}")
        self.log.debug(f"    ADC(1:0) I TVALID  : {r.STATUS_RFDC_DB0_ADC_I_TVALID:02b}")
        self.log.debug(f"    ADC(1:0) Q TREADY  : {r.STATUS_RFDC_DB0_ADC_I_TREADY:02b}")
        self.log.debug(f"    ADC(1:0) Q TVALID  : {r.STATUS_RFDC_DB0_ADC_I_TVALID:02b}")
        self.log.debug(f"  @USER")
        self.log.debug(f"    ADC(1:0) OUT TVALID: {r.STATUS_USER_DB0_ADC_OUT_TREADY:02b}")
        self.log.debug(f"    ADC(1:0) OUT TREADY: {r.STATUS_USER_DB0_ADC_OUT_TVALID:02b}")
        self.log.debug(f"Daughterboard 1")
        self.log.debug(f"  @RFDC")
        self.log.debug(f"    DAC(1:0) TREADY    : {r.STATUS_RFDC_DB1_DAC_TREADY:02b}")
        self.log.debug(f"    DAC(1:0) TVALID    : {r.STATUS_RFDC_DB1_DAC_TVALID:02b}")
        self.log.debug(f"    ADC(1:0) I TREADY  : {r.STATUS_RFDC_DB1_ADC_I_TREADY:02b}")
        self.log.debug(f"    ADC(1:0) I TVALID  : {r.STATUS_RFDC_DB1_ADC_I_TVALID:02b}")
        self.log.debug(f"    ADC(1:0) Q TREADY  : {r.STATUS_RFDC_DB1_ADC_I_TREADY:02b}")
        self.log.debug(f"    ADC(1:0) Q TVALID  : {r.STATUS_RFDC_DB1_ADC_I_TVALID:02b}")
        self.log.debug(f"  @USER")
        self.log.debug(f"    ADC(1:0) OUT TVALID: {r.STATUS_USER_DB1_ADC_OUT_TREADY:02b}")
        self.log.debug(f"    ADC(1:0) OUT TREADY: {r.STATUS_USER_DB1_ADC_OUT_TVALID:02b}")
        # pylint: enable=f-string-without-interpolation
        # pylint: enable=invalid-name

    def reset_gearboxes(self):
        """ Resets the ADC and DAC gearboxes. """
        self._regs.RFDC_ADC_GEARBOX_RESET = 1
        self._regs.RFDC_DAC_GEARBOX_RESET = 1
        self._commit()
        self._regs.RFDC_ADC_GEARBOX_RESET = 0
        self._regs.RFDC_DAC_GEARBOX_RESET = 0
        self._commit()

    ###########################################################################
    # Internal helpers
    ###########################################################################
    def _poke(self, addr, val):
        """Shorthand for self.regs.poke32"""
        with self.regs:
            self.regs.poke32(addr, val)

    def _peek(self, addr):
        """Shorthand for self.regs.peek32"""
        with self.regs:
            result = self.regs.peek32(addr)
            return result

    def _commit(self):
        """
        Write all registers that have a changed state.
        """
        for addr in self._regs.get_changed_addrs():
            self._poke(addr, self._regs.get_reg(addr))
        self._regs.save_state()

    def _update_reg(self, reg_name, idx=0):
        """
        Update the saved state of a register from the hardware
        """
        addr = self._regs.get_addr(reg_name)
        reg_val = self._peek(addr + 4 * idx)
        self._regs.set_reg(addr + 4 * idx, reg_val)
