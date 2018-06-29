#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
ADF400x driver class

Compatible with ADF4001 and ADF4002.
"""

from builtins import object
from usrp_mpm.mpmlog import get_logger


BASE_REF_CLOCK_FREQ = 40e6
DEFAULT_REF_CLOCK_FREQ = 20e6

class ADF400x(object):
    """
    Generic driver class for ADF4002 access.

    Inputs:
    freq : frequency of reference input
    parent_log : logger of parent
    """
    def __init__(self, regs_iface, freq=None, parent_log=None):
        self.log = \
            parent_log.getChild("ADF400x") if parent_log is not None \
            else get_logger("ADF400x")
        self.regs_iface = regs_iface
        assert hasattr(self.regs_iface, 'transfer24_8')
        self.transfer24_8 = regs_iface.transfer24_8

        # Instantiate our own copy of the register mapping and update some values
        self.adf400x_regs = ADF400xRegs()
        self.adf400x_regs.ref_counter = 1
        self.adf400x_regs.charge_pump_current_1 = 7
        self.adf400x_regs.charge_pump_current_2 = 7
        self.adf400x_regs.muxout = ADF400xRegs.MUXOUT_DLD
        self.adf400x_regs.counter_reset = ADF400xRegs.COUNTER_RESET_NORMAL
        self.adf400x_regs.phase_detector_polarity = ADF400xRegs.PHASE_DETECTOR_POLARITY_POS
        self.adf400x_regs.charge_pump_mode = ADF400xRegs.CHARGE_PUMP_TRISTATE
        # Set the N counter
        if freq is None:
            freq = DEFAULT_REF_CLOCK_FREQ
        self._set_n_counter(freq)

        # Now initialize the ADF400x
        self.program_regs()

    def program_regs(self):
        """
        Run through the programming sequence
        """
        # No control over CE, only LE, therefore we use the initialization latch method
        self._write_reg(3)
        # Conduct a function latch (2)
        self._write_reg(2)
        # Write R counter latch (0)
        self._write_reg(0)
        # Write N counter latch (1)
        self._write_reg(1)

    def _write_reg(self, addr):
        """Write the expected value to the given addr"""
        reg_val = self.adf400x_regs.get_reg(addr)
        self.log.trace("Writing {:06x} to spidev".format(reg_val))
        self.transfer24_8(reg_val)

    def set_lock_to_ext_ref(self, external):
        """Set the clock source to external"""
        if bool(external):
            self.adf400x_regs.charge_pump_mode = ADF400xRegs.CHARGE_PUMP_NORMAL
        else:
            self.adf400x_regs.charge_pump_mode = ADF400xRegs.CHARGE_PUMP_TRISTATE
        self.program_regs()

    def _set_n_counter(self, freq):
        n_counter = int(BASE_REF_CLOCK_FREQ / freq)
        if self.adf400x_regs.n_counter == n_counter:
            self.log.trace("No change to N counter value ({}); returning early".format(n_counter))
            return
        self.log.trace("Setting N counter to {}".format(n_counter))
        # Limits from the datasheet
        assert 1 <= n_counter <= 8191
        self.adf400x_regs.n_counter = n_counter

    def set_ref_freq(self, freq):
        """Set the input reference frequency"""
        self._set_n_counter(freq)
        self.program_regs()


class ADF400xRegs(object):
    """Register map for ADF400x"""
    # TODO: Move each field into an Enum or something
    # TODO: Add setters/getters for each field
    # anti backlash widths
    ANTI_BACKLASH_WIDTH_2_9NS = 0
    ANTI_BACKLASH_WIDTH_1_3NS = 1
    ANTI_BACKLASH_WIDTH_6_0NS = 2
    ANTI_BACKLASH_WIDTH_2_9NS_WAT = 3

    # lock detect precision
    LOCK_DETECT_PRECISION_3CYC = 0
    LOCK_DETECT_PRECISION_5CYC = 1

    # charge pump gain
    CHARGE_PUMP_GAIN_1 = 0
    CHARGE_PUMP_GAIN_2 = 1

    # counter reset
    COUNTER_RESET_NORMAL = 0
    COUNTER_RESET_RESET = 1

    # power down
    POWER_DOWN_NORMAL = 0
    POWER_DOWN_ASYNC = 1
    POWER_DOWN_SYNC = 3

    # muxout
    MUXOUT_TRISTATE_OUT = 0
    MUXOUT_DLD = 1
    MUXOUT_NDIV = 2
    MUXOUT_AVDD = 3
    MUXOUT_RDIV = 4
    MUXOUT_NCH_OD_ALD = 5
    MUXOUT_SDO = 6
    MUXOUT_GND = 7

    # phase detector polarity
    PHASE_DETECTOR_POLARITY_NEG = 0
    PHASE_DETECTOR_POLARITY_POS = 1

    # charge pump mode
    CHARGE_PUMP_NORMAL = 0
    CHARGE_PUMP_TRISTATE = 1

    # fastlock mode
    FASTLOCK_MODE_DISABLED = 0
    FASTLOCK_MODE_1 = 1
    FASTLOCK_MODE_2 = 2

    # timer counter control
    TIMEOUT_3CYC = 0
    TIMEOUT_7CYC = 1
    TIMEOUT_11CYC = 2
    TIMEOUT_15CYC = 3
    TIMEOUT_19CYC = 4
    TIMEOUT_23CYC = 5
    TIMEOUT_27CYC = 6
    TIMEOUT_31CYC = 7
    TIMEOUT_35CYC = 8
    TIMEOUT_39CYC = 9
    TIMEOUT_43CYC = 10
    TIMEOUT_47CYC = 11
    TIMEOUT_51CYC = 12
    TIMEOUT_55CYC = 13
    TIMEOUT_59CYC = 14
    TIMEOUT_63CYC = 15

    def __init__(self):
        """Set the default configuration"""
        self.ref_counter = 0
        self.n_counter = 0
        self.charge_pump_current_1 = 0
        self.charge_pump_current_2 = 0
        self.anti_backlash_width = ADF400xRegs.ANTI_BACKLASH_WIDTH_2_9NS
        self.lock_detect_precision = ADF400xRegs.LOCK_DETECT_PRECISION_3CYC
        self.charge_pump_gain = ADF400xRegs.CHARGE_PUMP_GAIN_1
        self.counter_reset = ADF400xRegs.COUNTER_RESET_NORMAL
        self.power_down = ADF400xRegs.POWER_DOWN_NORMAL
        self.muxout = ADF400xRegs.MUXOUT_TRISTATE_OUT
        self.phase_detector_polarity = ADF400xRegs.PHASE_DETECTOR_POLARITY_NEG
        self.charge_pump_mode = ADF400xRegs.CHARGE_PUMP_TRISTATE
        self.fastlock_mode = ADF400xRegs.FASTLOCK_MODE_DISABLED
        self.timer_counter_control = ADF400xRegs.TIMEOUT_3CYC

    def get_reg(self, addr):
        """Get the register value to write to the given addr"""
        reg = 0
        if addr == 0:
            reg |= (self.ref_counter & 0x003FFF) << 2
            reg |= (self.anti_backlash_width & 0x000003) << 16
            reg |= (self.lock_detect_precision & 0x000001) << 20
        elif addr == 1:
            reg |= (self.n_counter & 0x001FFF) << 8
            reg |= (self.charge_pump_gain & 0x000001) << 21
        elif addr == 2:
            reg |= (self.counter_reset & 0x000001) << 2
            reg |= (self.power_down & 0x000001) << 3
            reg |= (self.muxout & 0x000007) << 4
            reg |= (self.phase_detector_polarity & 0x000001) << 7
            reg |= (self.charge_pump_mode & 0x000001) << 8
            reg |= (self.fastlock_mode & 0x000003) << 9
            reg |= (self.timer_counter_control & 0x00000F) << 11
            reg |= (self.charge_pump_current_1 & 0x000007) << 15
            reg |= (self.charge_pump_current_2 & 0x000007) << 18
            reg |= (self.power_down & 0x000002) << 20
        elif addr == 3:
            reg |= (self.counter_reset & 0x000001) << 2
            reg |= (self.power_down & 0x000001) << 3
            reg |= (self.muxout & 0x000007) << 4
            reg |= (self.phase_detector_polarity & 0x000001) << 7
            reg |= (self.charge_pump_mode & 0x000001) << 8
            reg |= (self.fastlock_mode & 0x000003) << 9
            reg |= (self.timer_counter_control & 0x00000F) << 11
            reg |= (self.charge_pump_current_1 & 0x000007) << 15
            reg |= (self.charge_pump_current_2 & 0x000007) << 18
            reg |= (self.power_down & 0x000002) << 20
        reg |= addr
        return reg
