#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Magnesium dboard peripherals (CPLD, port expander, dboard regs)
"""

import time
from usrp_mpm.sys_utils.sysfs_gpio import SysFSGPIO
from usrp_mpm.mpmutils import poll_with_timeout

class TCA6408(object):
    """
    Abstraction layer for the port/gpio expander
    """
    pins = (
        'PWR-GOOD-3.6V', #3.6V
        'PWR-EN-3.6V',   #3.6V
        'PWR-GOOD-1.5V', #1.5V
        'PWR-EN-1.5V',   #1.5V
        'PWR-GOOD-5.5V', #5.5V
        'PWR-EN-5.5V',   #5.5V
        '6',
        'LED',
    )

    def __init__(self, i2c_dev):
        if i2c_dev is None:
            raise RuntimeError("Need to specify i2c device to use the TCA6408")
        self._gpios = SysFSGPIO('tca6408', 0xBF, 0xAA, 0xAA, i2c_dev)

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
        self.log.trace("Enabling FPGA Radio Clock MMCM...")
        self.poke32(self.RADIO_CLK_MMCM, 0x2)
        if not poll_with_timeout(
                lambda: bool(self.peek32(self.RADIO_CLK_MMCM) & 0x10),
                500,
                10,
            ):
            self.log.error("FPGA Radio Clock MMCM not locked!")
            raise RuntimeError("FPGA Radio Clock MMCM not locked!")
        self.log.trace("Radio Clock MMCM locked. Enabling clocks to design...")
        self.enable_outputs(True)

    def check_refclk(self):
        """
        Not technically a clocking reg, but related.
        """
        return bool(self.peek32(self.MGT_REF_CLK_STATUS) & 0x1)

class MgCPLD(object):
    """
    Control class for the CPLD
    """
    CPLD_SIGNATURE = 0xCAFE # Expected signature ("magic number")
    CPLD_MINOR_REV = 0
    CPLD_MAJOR_REV = 5

    REG_SIGNATURE = 0x0000
    REG_MINOR_REVISION = 0x0001
    REG_MAJOR_REVISION = 0x0002
    REG_BUILD_CODE_LSB = 0x0003
    REG_BUILD_CODE_MSB = 0x0004
    REG_SCRATCH   = 0x0005
    REG_CPLD_CTRL = 0x0010
    REG_LMK_CTRL  = 0x0011
    REG_LO_STATUS = 0x0012
    REG_MYK_CTRL  = 0x0013

    def __init__(self, regs, log):
        self.log = log.getChild("CPLD")
        self.log.debug("Initializing CPLD...")
        self.regs = regs
        self.poke16 = self.regs.poke16
        self.peek16 = self.regs.peek16
        signature = self.peek16(self.REG_SIGNATURE)
        if signature != self.CPLD_SIGNATURE:
            self.log.error(
                "CPLD Signature Mismatch! " \
                "Expected: 0x{:04X} Got: 0x{:04X}".format(
                    self.CPLD_SIGNATURE, signature))
            raise RuntimeError("CPLD Signature Check Failed! "
                               "Incorrect signature readback.")
        self.minor_rev = self.peek16(self.REG_MINOR_REVISION)
        self.major_rev = self.peek16(self.REG_MAJOR_REVISION)
        if self.major_rev != self.CPLD_MAJOR_REV:
            self.log.error(
                "CPLD Major Revision check mismatch! Expected: %d Got: %d",
                self.CPLD_MAJOR_REV,
                self.major_rev
            )
            raise RuntimeError("CPLD Revision Check Failed! MPM is not "
                               "compatible with the loaded CPLD image.")
        date_code = self.peek16(self.REG_BUILD_CODE_LSB) | \
                    (self.peek16(self.REG_BUILD_CODE_MSB) << 16)
        self.log.debug(
            "CPLD Signature: 0x{:04X} "
            "Revision: {}.{} "
            "Date code: 0x{:08X}"
            .format(signature, self.major_rev, self.minor_rev, date_code))

    def set_scratch(self, val):
        " Write to the scratch register "
        self.poke16(self.REG_SCRATCH, val & 0xFFFF)

    def get_scratch(self):
        " Read from the scratch register "
        return self.peek16(self.REG_SCRATCH)

    def reset(self):
        " Reset entire CPLD "
        self.log.trace("Resetting CPLD...")
        self.poke16(self.REG_CPLD_CTRL, 0x1)
        self.poke16(self.REG_CPLD_CTRL, 0x0)

    def set_pdac_control(self, enb):
        """
        If enb is True, the Phase DAC will exclusively control the VCXO voltage
        """
        self.log.trace("Giving Phase %s control over VCXO voltage...",
                       "exclusive" if bool(enb) else "non-exclusive")
        reg_val = (1<<4) if enb else 0
        self.poke16(self.REG_LMK_CTRL, reg_val)

    def get_lo_lock_status(self, which):
        """
        Returns True if the 'which' LO is locked. 'which' is either 'tx' or
        'rx'.
        """
        mask = (1<<4) if which.lower() == 'tx' else 1
        return bool(self.peek16(self.REG_LO_STATUS & mask))

    def reset_mykonos(self):
        """
        Hard-resets Mykonos
        """
        self.log.debug("Resetting AD9371!")
        self.poke16(self.REG_MYK_CTRL, 0x1)
        time.sleep(0.001) # No spec here, but give it some time to reset.
        self.poke16(self.REG_MYK_CTRL, 0x0)
        time.sleep(0.001) # No spec here, but give it some time to reset.

