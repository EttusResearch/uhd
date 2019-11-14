#
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E310 peripherals
"""

from usrp_mpm.sys_utils.sysfs_gpio import SysFSGPIO, GPIOBank
from usrp_mpm.periph_manager.common import MboardRegsCommon

# pylint: disable=too-few-public-methods
class FrontpanelGPIO(GPIOBank):
    """
    Abstraction layer for the front panel GPIO
    """
    EMIO_BASE = 54
    FP_GPIO_OFFSET = 32 # Bit offset within the ps_gpio_* pins

    def __init__(self, ddr):
        GPIOBank.__init__(
            self,
            {'label': 'zynq_gpio'},
            self.FP_GPIO_OFFSET + self.EMIO_BASE,
            0xFF, # use_mask
            ddr
        )
# pylint: enable=too-few-public-methods

class MboardRegsControl(MboardRegsCommon):
    """
    Control the FPGA Motherboard registers
    """
    # pylint: disable=bad-whitespace
    # Motherboard registers
    MB_CLOCK_CTRL       = 0x0018
    MB_XADC_RB          = 0x001C
    MB_BUS_CLK_RATE     = 0x0020
    MB_BUS_COUNTER      = 0x0024
    MB_SFP_PORT_INFO    = 0x0028
    MB_GPIO_CTRL        = 0x002C
    MB_GPIO_MASTER      = 0x0030
    MB_GPIO_RADIO_SRC   = 0x0034
    MB_GPS_CTRL         = 0x0038
    MB_GPS_STATUS       = 0x003C
    MB_DBOARD_CTRL      = 0x0040
    MB_DBOARD_STATUS    = 0x0044

    # PPS select values for MB_CLOCK_CTRL (for reading and writing)
    MB_CLOCK_CTRL_PPS_SEL_GPS = 0
    # Note: 1 is also valid, but we've always used 2 in SW so let's keep doing that
    MB_CLOCK_CTRL_PPS_SEL_INT = 2
    MB_CLOCK_CTRL_PPS_SEL_INT_ALT = 1
    MB_CLOCK_CTRL_PPS_SEL_EXT = 3
    # Bitfield locations for the MB_CLOCK_CTRL register.
    MB_CLOCK_CTRL_REF_CLK_LOCKED = 3

    # Bitfield locations for the MB_DBOARD_CTRL register.
    MB_DBOARD_CTRL_MIMO = 0
    MB_DBOARD_CTRL_TX_CHAN_SEL = 1

    # Bitfield locations for the MB_DBOARD_STATUS register.
    MB_DBOARD_STATUS_RX_LOCK = 6
    MB_DBOARD_STATUS_TX_LOCK = 7
    # pylint: enable=bad-whitespace

    def __init__(self, label, log):
        MboardRegsCommon.__init__(self, label, log)

    def set_fp_gpio_master(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is a single bit bit mask of 6 pins GPIO
        """
        with self.regs:
            return self.poke32(self.MB_GPIO_MASTER, value)

    def get_fp_gpio_master(self):
        """get "who" is driving front panel gpio
           The return value is a bit mask of 6 pins GPIO.
           0: means the pin is driven by PL
           1: means the pin is driven by PS
        """
        with self.regs:
            return self.peek32(self.MB_GPIO_MASTER) & 0xff

    def set_fp_gpio_radio_src(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is 2-bit bit mask of 6 pins GPIO
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
        """
        with self.regs:
            return self.poke32(self.MB_GPIO_RADIO_SRC, value)

    def get_fp_gpio_radio_src(self):
        """get which radio is driving front panel gpio
           The return value is 2-bit bit mask of 6 pins GPIO.
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
        """
        with self.regs:
            return self.peek32(self.MB_GPIO_RADIO_SRC) & 0xfff

    def set_time_source(self, time_source):
        """
        Set time source
        """
        pps_sel_val = 0x0
        if time_source == 'internal':
            self.log.trace("Setting time source to internal")
            pps_sel_val = self.MB_CLOCK_CTRL_PPS_SEL_INT
        elif time_source == 'gpsdo':
            self.log.trace("Setting time source to gpsdo...")
            pps_sel_val = self.MB_CLOCK_CTRL_PPS_SEL_GPS
        elif time_source == 'external':
            self.log.trace("Setting time source to external...")
            pps_sel_val = self.MB_CLOCK_CTRL_PPS_SEL_EXT
        else:
            assert False, "Cannot set to invalid time source: {}".format(time_source)
        with self.regs:
            reg_val = self.peek32(self.MB_CLOCK_CTRL) & 0xFFFFFF90
            # prevent glitches by writing a cleared value first, then the final value.
            self.poke32(self.MB_CLOCK_CTRL, reg_val)
            reg_val = reg_val | (pps_sel_val & 0x6F)
            self.log.trace("Writing MB_CLOCK_CTRL to 0x{:08X}".format(reg_val))
            self.poke32(self.MB_CLOCK_CTRL, reg_val)

    def get_refclk_lock(self):
        """
        Check the status of the reference clock in FPGA.
        """
        mask = 0b1 << self.MB_CLOCK_CTRL_REF_CLK_LOCKED
        with self.regs:
            reg_val = self.peek32(self.MB_CLOCK_CTRL)
        locked = (reg_val & mask) > 0
        self.log.trace("Reference Clock %slocked!", "" if locked else "un")
        return locked

    def set_channel_mode(self, channel_mode):
        """
        Set channel mode in FPGA and select which tx channel to use
        channel mode = "MIMO" for mimo
        channel mode = "SISO_TX1", "SISO_TX0" for siso tx1, tx0 respectively.
        """
        with self.regs:
            reg_val = self.peek32(self.MB_DBOARD_CTRL)
            if channel_mode == "MIMO":
                reg_val = (0b1 << self.MB_DBOARD_CTRL_MIMO)
                self.log.trace("Setting channel mode in AD9361 interface: %s",
                               "2R2T" if channel_mode == 2 else "1R1T")
            else:
                # Warn if user tries to set either tx0/tx1 in mimo mode
                # as both will be set automatically
                if channel_mode == "SISO_TX1":
                    # in SISO mode, Channel 1
                    reg_val = (0b1 << self.MB_DBOARD_CTRL_TX_CHAN_SEL) | (0b0 << self.MB_DBOARD_CTRL_MIMO)
                    self.log.trace("Setting TX channel in AD9361 interface to: TX1")
                elif channel_mode == "SISO_TX0":
                    # in SISO mode, Channel 0
                    reg_val = (0b0 << self.MB_DBOARD_CTRL_TX_CHAN_SEL) | (0b0 << self.MB_DBOARD_CTRL_MIMO)
                    self.log.trace("Setting TX channel in AD9361 interface to: TX0")
            self.log.trace("Writing MB_DBOARD_CTRL to 0x{:08X}".format(reg_val))
            self.poke32(self.MB_DBOARD_CTRL, reg_val)

    def get_ad9361_tx_lo_lock(self):
        """
        Check the status of TX LO lock from CTRL_OUT pins from Catalina
        """
        mask = 0b1 << self.MB_DBOARD_STATUS_TX_LOCK
        with self.regs:
            reg_val = self.peek32(self.MB_DBOARD_STATUS)
        locked = (reg_val & mask) > 0
        if not locked:
            self.log.warning("TX RF PLL reporting unlocked. ")
        else:
            self.log.trace("TX RF PLL locked")
        return locked

    def get_ad9361_rx_lo_lock(self):
        """
        Check the status of RX LO lock from CTRL_OUT pins from the RFIC
        """
        mask = 0b1 << self.MB_DBOARD_STATUS_RX_LOCK
        with self.regs:
            reg_val = self.peek32(self.MB_DBOARD_STATUS)
        locked = (reg_val & mask) > 0
        if not locked:
            self.log.warning("RX RF PLL reporting unlocked. ")
        else:
            self.log.trace("RX RF PLL locked")
        return locked
