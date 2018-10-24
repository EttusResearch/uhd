#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Rhodium dboard peripherals (CPLD, port expander, dboard regs)
"""


import time
from usrp_mpm.sys_utils.sysfs_gpio import SysFSGPIO, GPIOBank
from usrp_mpm.mpmutils import poll_with_timeout


class TCA6408(object):
    """
    Abstraction layer for the port/gpio expander
    """
    pins = (
        'PWR-GOOD-3.6V', #3.6V
        'PWR-GOOD-1.1V', #1.1V
        'PWR-GOOD-2.0V', #2.0V
        'PWR-GOOD-5.4V', #5.4V
        'PWR-GOOD-5.5V', #6.8V
    )

    def __init__(self, i2c_dev):
        if i2c_dev is None:
            raise RuntimeError("Need to specify i2c device to use the TCA6408")
        self._gpios = SysFSGPIO({'label': 'tca6408'}, 0x3F, 0x00, 0x00, i2c_dev)

    def set(self, name, value=None):
        """
        Assert a pin by name
        """
        assert name in self.pins
        self._gpios.set(self.pins.index(name), value=value)

    def reset(self, name):
        """
        Deassert a pin by name
        """
        self.set(name, value=0)

    def get(self, name):
        """
        Read back a pin by name
        """
        assert name in self.pins
        return self._gpios.get(self.pins.index(name))


class FPGAtoDbGPIO(GPIOBank):
    """
    Abstraction layer for the FPGA to Daughterboard GPIO
    """
    EMIO_BASE = 54+8
    DB_POWER_ENABLE = 0
    RF_POWER_ENABLE = 1

    def __init__(self, slot_idx):
        pwr_base = self.EMIO_BASE + 2*slot_idx
        GPIOBank.__init__(
            self,
            {'label': 'zynq_gpio'},
            pwr_base,
            0x3, # use_mask
            0x3, # ddr
        )


class RhCPLD(object):
    """
    Control class for the CPLD.
    """

    REG_SIGNATURE      = 0x0000
    REG_MINOR_REVISION = 0x0001
    REG_MAJOR_REVISION = 0x0002
    REG_BUILD_CODE_LSB = 0x0003
    REG_BUILD_CODE_MSB = 0x0004
    REG_SCRATCH        = 0x0005
    REG_GAIN_TABLE_SEL = 0x0006
    REG_DAC_ALARM      = 0x0007

    CPLD_SIGNATURE = 0x0045
    CPLD_MAJOR_REV = 4
    CPLD_MINOR_REV = 0

    def __init__(self, regs, log):
        """
        Initialize communication with the Rh CPLD
        """
        self.log = log.getChild("CPLD")
        self.log.debug("Initializing CPLD...")
        self.cpld_regs = regs
        self.poke16 = self.cpld_regs.peek16
        self.peek16 = self.cpld_regs.peek16
        # According to the datasheet, The CPLD's internal configuration time can take
        # anywhere from 0.6 to 3.4 ms. Until then, any ifc accesses will be invalid,
        # We will blind wait 5 ms and check the signature register to verify operation
        time.sleep(0.005)
        signature = self.peek16(self.REG_SIGNATURE)
        if self.CPLD_SIGNATURE != signature:
            self.log.error("CPLD Signature Mismatch! " \
                "Expected: 0x{:04X} Got: 0x{:04X}".format(self.CPLD_SIGNATURE, signature))
            raise RuntimeError("CPLD Signature Check Failed! "
                               "Incorrect signature readback.")
        minor_rev = self.peek16(self.REG_MINOR_REVISION)
        major_rev = self.peek16(self.REG_MAJOR_REVISION)
        if major_rev != self.CPLD_MAJOR_REV:
            self.log.error(
                "CPLD Major Revision check mismatch! Expected: %d Got: %d",
                self.CPLD_MAJOR_REV,
                major_rev
            )
            raise RuntimeError("CPLD Revision Check Failed! MPM is not "
                               "compatible with the loaded CPLD image.")
        date_code = self.peek16(self.REG_BUILD_CODE_LSB) | \
                    (self.peek16(self.REG_BUILD_CODE_MSB) << 16)
        self.log.debug(
            "CPLD Signature: 0x{:04X} "
            "Revision: {}.{} "
            "Date code: 0x{:08X}"
            .format(signature, major_rev, minor_rev, date_code))

    def get_dac_alarm(self):
        """
        This function polls and returns the DAC's ALARM signal connected to the CPLD.
        """
        return (self.peek16(self.REG_DAC_ALARM) & 0x0001)

    # TODO: add more control/status functionality to this class?


class DboardClockControl(object):
    """
    Control the FPGA MMCM for Radio Clock control.
    """
    # Clocking Register address constants
    RADIO_CLK_MMCM      = 0x0020
    PHASE_SHIFT_CONTROL = 0x0024
    RADIO_CLK_ENABLES   = 0x0028
    MGT_REF_CLK_STATUS  = 0x0030

    def __init__(self, regs, log):
        self.log = log
        self.regs = regs
        self.poke32 = self.regs.poke32
        self.peek32 = self.regs.peek32

    def enable_outputs(self, enable=True):
        """
        Enables or disables the MMCM outputs.
        """
        if enable:
            self.poke32(self.RADIO_CLK_ENABLES, 0x011)
        else:
            self.poke32(self.RADIO_CLK_ENABLES, 0x000)

    def reset_mmcm(self):
        """
        Uninitialize and reset the MMCM
        """
        self.log.trace("Disabling all Radio Clocks, then resetting MMCM...")
        self.enable_outputs(False)
        self.poke32(self.RADIO_CLK_MMCM, 0x1)

    def enable_mmcm(self):
        """
        Unreset MMCM and poll lock indicators

        If MMCM is not locked after unreset, an exception is thrown.
        """
        self.log.trace("Un-resetting MMCM...")
        self.poke32(self.RADIO_CLK_MMCM, 0x2)
        if not poll_with_timeout(
                 lambda: bool(self.peek32(self.RADIO_CLK_MMCM) & 0x10),
                 500,
                 10,
                ):
            self.log.error("MMCM not locked!")
            raise RuntimeError("MMCM not locked!")
        self.log.trace("MMCM locked. Enabling output MMCM clocks...")
        self.enable_outputs(True)

    def check_refclk(self):
        """
        Not technically a clocking reg, but related.
        """
        return bool(self.peek32(self.MGT_REF_CLK_STATUS) & 0x1)

