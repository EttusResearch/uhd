#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4xx RFDC register control
"""

import time
from usrp_mpm.sys_utils.uio import UIO

class RfdcRegsControl:
    """
    Control the FPGA RFDC registers external to the XRFdc API
    """
    # pylint: disable=bad-whitespace
    IQ_SWAP_OFFSET          = 0x10000
    MMCM_RESET_BASE_OFFSET  = 0x11000
    RF_RESET_CONTROL_OFFSET = 0x12000
    RF_RESET_STATUS_OFFSET  = 0x12008
    RF_STATUS_OFFSET        = 0x13000
    FABRIC_DSP_INFO_OFFSET  = 0x13008
    CAL_DATA_OFFSET         = 0x14000
    CAL_ENABLE_OFFSET       = 0x14008
    THRESHOLD_STATUS_OFFSET = 0x15000
    RF_PLL_CONTROL_OFFSET   = 0x16000
    RF_PLL_STATUS_OFFSET    = 0x16008
    # pylint: enable=bad-whitespace

    def __init__(self, label, log):
        self.log = log.getChild("RfdcRegs")
        self.regs = UIO(
            label=label,
            read_only=False
        )
        self.poke32 = self.regs.poke32
        self.peek32 = self.regs.peek32

        # Index corresponds to dboard number.
        self._converter_chains_in_reset = True

    def get_threshold_status(self, slot_id, channel, threshold_idx):
        """
        Retrieves the status bit for the given threshold block
        """
        BITMASKS = {
            (0, 0, 0): 0x04,
            (0, 0, 1): 0x08,
            (0, 1, 0): 0x01,
            (0, 1, 1): 0x02,
            (1, 0, 0): 0x400,
            (1, 0, 1): 0x800,
            (1, 1, 0): 0x100,
            (1, 1, 1): 0x200,
        }
        assert (slot_id, channel, threshold_idx) in BITMASKS
        status = self.peek(self.THRESHOLD_STATUS_OFFSET)
        status_bool = (status & BITMASKS[(slot_id, channel, threshold_idx)]) != 0
        return 1 if status_bool else 0

    def set_cal_data(self, i, q):
        assert 0 <= i < 2**16
        assert 0 <= q < 2**16
        self.poke(self.CAL_DATA_OFFSET, (q << 16) | i)

    def set_cal_enable(self, channel, enable):
        assert 0 <= channel <= 3
        assert enable in [False, True]
        en = self.peek(self.CAL_ENABLE_OFFSET)
        bit_offsets = {
            0: 0,
            1: 1,
            2: 4,
            3: 5,
        }
        en_mask = 1 << bit_offsets[channel]
        en = en & ~en_mask
        self.poke(self.CAL_ENABLE_OFFSET, en | (en_mask if enable else 0))

    def enable_iq_swap(self, enable, db_id, block_id, is_dac):
        iq_swap_bit = (int(is_dac) * 8) + (db_id * 4) + block_id

        # Write IQ swap bit with a mask
        reg_val = self.peek(self.IQ_SWAP_OFFSET)
        reg_val = (reg_val & ~(1 << iq_swap_bit)) \
                    | (enable << iq_swap_bit)
        self.poke(self.IQ_SWAP_OFFSET, reg_val)

    def set_reset_mmcm(self, reset=True):
        if reset:
            # Put the MMCM in reset (active low)
            self.poke(self.MMCM_RESET_BASE_OFFSET, 0)
        else:
            # Take the MMCM out of reset
            self.poke(self.MMCM_RESET_BASE_OFFSET, 1)

    def wait_for_mmcm_locked(self, timeout=0.001):
        """
        Wait for MMCM to come to a stable locked state.
        The datasheet specifies a 100us max lock time
        """
        DATA_CLK_PLL_LOCKED = 1 << 20

        POLL_SLEEP = 0.0002
        for _ in range(int(timeout / POLL_SLEEP)):
            time.sleep(POLL_SLEEP)
            status = self.peek(self.RF_PLL_STATUS_OFFSET)
            if status & DATA_CLK_PLL_LOCKED:
                self.log.trace("RF MMCM lock detected.")
                return
        self.log.error("MMCM failed to lock in the expected time.")
        raise RuntimeError("MMCM failed to lock within the expected time.")

    def set_gated_clock_enables(self, value=True):
        """
        Controls the clock enable for data_clk and
        data_clk_2x
        """
        ENABLE_DATA_CLK     = 1
        ENABLE_DATA_CLK_2X  = 1 << 4
        ENABLE_RF_CLK       = 1 << 8
        ENABLE_RF_CLK_2X    = 1 << 12
        if value:
            # Enable buffers gating the clocks
            self.poke(self.RF_PLL_CONTROL_OFFSET,
                ENABLE_DATA_CLK |
                ENABLE_DATA_CLK_2X |
                ENABLE_RF_CLK |
                ENABLE_RF_CLK_2X
            )
        else:
            # Disable clock buffers to have clocks gated.
            self.poke(self.RF_PLL_CONTROL_OFFSET, 0)

    def get_fabric_dsp_info(self, dboard):
        """
        Read the DSP information register and returns the
        DSP bandwidth, rx channel count and tx channel count
        """
        # Offsets
        DSP_BW     = 0 + 16*dboard
        DSP_RX_CNT = 12 + 16*dboard
        DSP_TX_CNT = 14 + 16*dboard
        # Masks
        DSP_BW_MSK = 0xFFF
        DSP_RX_CNT_MSK = 0x3
        DSP_TX_CNT_MSK = 0x3

        dsp_info = self.peek(self.FABRIC_DSP_INFO_OFFSET)
        self.log.trace("Fabric DSP for dboard %d...", dboard)
        dsp_bw = (dsp_info >> DSP_BW) & DSP_BW_MSK
        self.log.trace("  Bandwidth (MHz):  %d", dsp_bw)
        dsp_rx_cnt = (dsp_info >> DSP_RX_CNT) & DSP_RX_CNT_MSK
        self.log.trace("  Rx channel count: %d", dsp_rx_cnt)
        dsp_tx_cnt = (dsp_info >> DSP_TX_CNT) & DSP_TX_CNT_MSK
        self.log.trace("  Tx channel count: %d", dsp_tx_cnt)

        return [dsp_bw, dsp_rx_cnt, dsp_tx_cnt]

    def get_rfdc_resampling_factor(self, dboard):
        """
        Returns the appropriate decimation/interpolation factor to set in the RFDC.
        """
        # DSP vs. RFDC decimation/interpolation dictionary
        # Key: bandwidth in MHz
        # Value: (RFDC resampling factor, is Half-band resampling used?)
        RFDC_RESAMPLING_FACTOR = {
            100: (8, False), # 100 MHz BW requires 8x RFDC resampling
            200: (2, True),  # 200 MHz BW requires 2x RFDC resampling
                             # (400 MHz RFDC DSP used w/ half-band resampling)
            400: (2, False)  # 400 MHz BW requires 2x RFDC resampling
        }
        dsp_bw, _, _ = self.get_fabric_dsp_info(dboard)
        # When no RF fabric DSP is present (dsp_bw = 0), MPM should
        # simply use the default RFDC resampling factor (400 MHz).
        if dsp_bw in RFDC_RESAMPLING_FACTOR:
            rfdc_resampling_factor, halfband = RFDC_RESAMPLING_FACTOR[dsp_bw]
        else:
            rfdc_resampling_factor, halfband = RFDC_RESAMPLING_FACTOR[400]
            self.log.trace("  Using default resampling!")
        self.log.trace("  RFDC resampling:  %d", rfdc_resampling_factor)
        return (rfdc_resampling_factor, halfband)

    def set_reset_adc_dac_chains(self, reset=True):
        """ Resets or enables the ADC and DAC chain for the given dboard """

        def _wait_for_done(done_bit, timeout=5):
            """
            Wait for the specified sequence done bit when resetting or
            enabling an ADC or DAC chain. Throws an error on timeout.
            """
            status = self.peek(self.RF_RESET_STATUS_OFFSET)
            if (status & done_bit):
                return
            for _ in range(0, timeout):
                time.sleep(0.001) # 1 ms
                status = self.peek(self.RF_RESET_STATUS_OFFSET)
                if (status & done_bit):
                    return
            self.log.error("Timeout while resetting or enabling ADC/DAC chains.")
            raise RuntimeError("Timeout while resetting or enabling ADC/DAC chains.")

        # CONTROL OFFSET
        ADC_RESET   = 1 << 4
        DAC_RESET   = 1 << 8
        # STATUS OFFSET
        ADC_SEQ_DONE    = 1 << 7
        DAC_SEQ_DONE    = 1 << 11

        if reset:
            if self._converter_chains_in_reset:
                self.log.debug('Converters are already in reset. '
                               'The reset bit will NOT be toggled.')
                return
            # Reset the ADC and DAC chains
            self.log.trace('Resetting ADC chain')
            self.poke(self.RF_RESET_CONTROL_OFFSET, ADC_RESET)
            _wait_for_done(ADC_SEQ_DONE)
            self.poke(self.RF_RESET_CONTROL_OFFSET, 0x0)

            self.log.trace('Resetting DAC chain')
            self.poke(self.RF_RESET_CONTROL_OFFSET, DAC_RESET)
            _wait_for_done(DAC_SEQ_DONE)
            self.poke(self.RF_RESET_CONTROL_OFFSET, 0x0)

            self._converter_chains_in_reset = True
        else: # enable
            self._converter_chains_in_reset = False

    def log_status(self):
        status = self.peek(self.RF_STATUS_OFFSET)
        self.log.debug("Daughterboard 0")
        self.log.debug("  @RFDC")
        self.log.debug("    DAC(1:0) TREADY    : {:02b}".format((status >> 0) & 0x3))
        self.log.debug("    DAC(1:0) TVALID    : {:02b}".format((status >> 2) & 0x3))
        self.log.debug("    ADC(1:0) I TREADY  : {:02b}".format((status >> 6) & 0x3))
        self.log.debug("    ADC(1:0) I TVALID  : {:02b}".format((status >> 10) & 0x3))
        self.log.debug("    ADC(1:0) Q TREADY  : {:02b}".format((status >> 4) & 0x3))
        self.log.debug("    ADC(1:0) Q TVALID  : {:02b}".format((status >> 8) & 0x3))
        self.log.debug("  @USER")
        self.log.debug("    ADC(1:0) OUT TVALID: {:02b}".format((status >> 12) & 0x3))
        self.log.debug("    ADC(1:0) OUT TREADY: {:02b}".format((status >> 14) & 0x3))
        self.log.debug("Daughterboard 1")
        self.log.debug("  @RFDC")
        self.log.debug("    DAC(1:0) TREADY    : {:02b}".format((status >> 16) & 0x3))
        self.log.debug("    DAC(1:0) TVALID    : {:02b}".format((status >> 18) & 0x3))
        self.log.debug("    ADC(1:0) I TREADY  : {:02b}".format((status >> 22) & 0x3))
        self.log.debug("    ADC(1:0) I TVALID  : {:02b}".format((status >> 26) & 0x3))
        self.log.debug("    ADC(1:0) Q TREADY  : {:02b}".format((status >> 20) & 0x3))
        self.log.debug("    ADC(1:0) Q TVALID  : {:02b}".format((status >> 24) & 0x3))
        self.log.debug("  @USER")
        self.log.debug("    ADC(1:0) OUT TVALID: {:02b}".format((status >> 28) & 0x3))
        self.log.debug("    ADC(1:0) OUT TREADY: {:02b}".format((status >> 30) & 0x3))

    def poke(self, addr, val):
        with self.regs:
            self.regs.poke32(addr, val)

    def peek(self, addr):
        with self.regs:
            result = self.regs.peek32(addr)
            return result
