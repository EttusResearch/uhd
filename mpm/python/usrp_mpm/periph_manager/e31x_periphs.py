#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
E310 peripherals
"""

import datetime
import math
from usrp_mpm.sys_utils.sysfs_gpio import SysFSGPIO, GPIOBank
from usrp_mpm.sys_utils.uio import UIO

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

class MboardRegsControl(object):
    """
    Control the FPGA Motherboard registers
    """
    # Motherboard registers
    MB_COMPAT_NUM     = 0x0000
    MB_DATESTAMP      = 0x0004
    MB_GIT_HASH       = 0x0008
    MB_SCRATCH        = 0x000C
    MB_NUM_CE         = 0x0010
    MB_NUM_IO_CE      = 0x0014
    MB_CLOCK_CTRL     = 0x0018
    MB_XADC_RB        = 0x001C
    MB_BUS_CLK_RATE   = 0x0020
    MB_BUS_COUNTER    = 0x0024
    MB_GPIO_MASTER    = 0x0030
    MB_GPIO_RADIO_SRC = 0x0034
    MB_GPS_CTRL       = 0x0038
    MB_GPS_STATUS     = 0x003C
    MB_DBOARD_CTRL    = 0x0040
    MB_DBOARD_STATUS  = 0x0044
    MB_XBAR_BASEPORT  = 0x0048

    # Bitfield locations for the MB_CLOCK_CTRL register.
    MB_CLOCK_CTRL_PPS_SEL_INT = 0
    MB_CLOCK_CTRL_PPS_SEL_EXT = 1
    MB_CLOCK_CTRL_REF_CLK_LOCKED = 2

    # Bitfield locations for the MB_GPS_CTRL register.
    #FIXME: Update for E310
    MB_GPS_CTRL_PWR_EN = 0
    MB_GPS_CTRL_RST_N = 1
    MB_GPS_CTRL_INITSURV_N = 2

    # Bitfield locations for the MB_GPS_STATUS register.
    #FIXME: Update for E310
    MB_GPS_STATUS_LOCK = 0
    MB_GPS_STATUS_ALARM = 1
    MB_GPS_STATUS_PHASELOCK = 2
    MB_GPS_STATUS_SURVEY = 3
    MB_GPS_STATUS_WARMUP = 4

    # Bitfield locations for the MB_DBOARD_CTRL register.
    MB_DBOARD_CTRL_MIMO = 0
    MB_DBOARD_CTRL_TX_CHAN_SEL = 1

    # Bitfield locations for the MB_DBOARD_STATUS register.
    MB_DBOARD_STATUS_RX_LOCK = 6
    MB_DBOARD_STATUS_TX_LOCK = 7

    def __init__(self, label, log):
        self.log = log
        self.regs = UIO(
            label=label,
            read_only=False
        )
        self.poke32 = self.regs.poke32
        self.peek32 = self.regs.peek32

    def get_compat_number(self):
        """get FPGA compat number

        This function reads back FPGA compat number.
        The return is a tuple of
        2 numbers: (major compat number, minor compat number )
        """
        with self.regs:
            compat_number = self.peek32(self.MB_COMPAT_NUM)
        minor = compat_number & 0xff
        major = (compat_number>>16) & 0xff
        return (major, minor)

    def set_fp_gpio_master(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is a single bit bit mask of 8 pins GPIO
        """
        with self.regs:
            return self.poke32(self.MB_GPIO_MASTER, value)

    def get_fp_gpio_master(self):
        """get "who" is driving front panel gpio
           The return value is a bit mask of 8 pins GPIO.
           0: means the pin is driven by PL
           1: means the pin is driven by PS
        """
        with self.regs:
            return self.peek32(self.MB_GPIO_MASTER) & 0xfff

    def set_fp_gpio_radio_src(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is 2-bit bit mask of 8 pins GPIO
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
        """
        with self.regs:
            return self.poke32(self.MB_GPIO_RADIO_SRC, value)

    def get_fp_gpio_radio_src(self):
        """get which radio is driving front panel gpio
           The return value is 2-bit bit mask of 8 pins GPIO.
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
        """
        with self.regs:
            return self.peek32(self.MB_GPIO_RADIO_SRC) & 0xffffff

    def get_build_timestamp(self):
        """
        Returns the build date/time for the FPGA image.
        The return is datetime string with the  ISO 8601 format
        (YYYY-MM-DD HH:MM:SS.mmmmmm)
        """
        with self.regs:
            datestamp_rb = self.peek32(self.MB_DATESTAMP)
        if datestamp_rb > 0:
            dt_str = datetime.datetime(
                year=((datestamp_rb>>17)&0x3F)+2000,
                month=(datestamp_rb>>23)&0x0F,
                day=(datestamp_rb>>27)&0x1F,
                hour=(datestamp_rb>>12)&0x1F,
                minute=(datestamp_rb>>6)&0x3F,
                second=((datestamp_rb>>0)&0x3F))
            self.log.trace("FPGA build timestamp: {}".format(str(dt_str)))
            return str(dt_str)
        else:
            # Compatibility with FPGAs without datestamp capability
            return ''

    def get_git_hash(self):
        """
        Returns the GIT hash for the FPGA build.
        The return is a tuple of
        2 numbers: (short git hash, bool: is the tree dirty?)
        """
        with self.regs:
            git_hash_rb = self.peek32(self.MB_GIT_HASH)
        git_hash = git_hash_rb & 0x0FFFFFFF
        tree_dirty = ((git_hash_rb & 0xF0000000) > 0)
        dirtiness_qualifier = 'dirty' if tree_dirty else 'clean'
        self.log.trace("FPGA build GIT Hash: {:07x} ({})".format(
            git_hash, dirtiness_qualifier))
        return (git_hash, dirtiness_qualifier)

    def set_time_source(self, time_source):
        """
        Set time source
        """
        pps_sel_val = 0x0
        if time_source == 'internal':
            self.log.trace("Setting time source to internal")
            pps_sel_val = self.MB_CLOCK_CTRL_PPS_SEL_INT
        elif time_source == 'gpsdo':
            self.log.debug("Setting time source to gpsdo...")
            pps_sel_val = self.MB_CLOCK_CTRL_PPS_SEL_GPS
        elif time_source == 'external':
            self.log.debug("Setting time source to external...")
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

    def set_clock_source(self, clock_source):
        """
        Set clock source
        """
        if clock_source == 'internal':
            self.log.trace("Setting clock source to internal")
        else:
            assert False, "Cannot set to invalid clock source: {}".format(clock_source)

    def get_fpga_type(self):
        """
        Reads the type of the FPGA image currently loaded
        Returns a string with the type (SG1, SG3)
        """
        #TODO: Add SG1 and SG3?
        return ""

    def get_gps_status(self):
        """
        Get GPS status
        """
        mask = 0x1F
        with self.regs:
            gps_status = self.peek32(self.MB_GPS_STATUS) & mask
        return gps_status

    def get_refclk_lock(self):
        """
        Check the status of the reference clock in FPGA.
        """
        mask = 0b1 << self.MB_CLOCK_CTRL_REF_CLK_LOCKED
        with self.regs:
            reg_val = self.peek32(self.MB_CLOCK_CTRL)
        locked = (reg_val & mask) > 0
        if not locked:
            self.log.warning("Reference Clock reporting unlocked. "
                             "MB_CLOCK_CTRL reg: 0x{:08X}".format(reg_val))
        else:
            self.log.trace("Reference Clock locked!")
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
                self.log.trace("Setting channel mode in AD9361 interface: {}".format("2R2T" if channel_mode == 2 else "1R1T"))
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
            reg_val =  self.peek32(self.MB_DBOARD_STATUS)
        locked = (reg_val & mask) > 0
        if not locked:
            self.log.warning("TX RF PLL reporting unlocked. ")
        else:
            self.log.trace("TX RF PLL locked")
        return locked

    def get_ad9361_rx_lo_lock(self):
        """
        Check the status of RX LO lock from CTRL_OUT pins from Catalina
        """
        mask = 0b1 << self.MB_DBOARD_STATUS_RX_LOCK
        with self.regs:
            reg_val =  self.peek32(self.MB_DBOARD_STATUS)
        locked = (reg_val & mask) > 0
        if not locked:
            self.log.warning("RX RF PLL reporting unlocked. ")
        else:
            self.log.trace("RX RF PLL locked")
        return locked

    def get_xbar_baseport(self):
        "Get the RFNoC crossbar base port"
        with self.regs:
            return self.peek32(self.MB_XBAR_BASEPORT)
