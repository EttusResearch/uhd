#
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
N3xx peripherals
"""

from usrp_mpm import lib
from usrp_mpm.sys_utils.sysfs_gpio import SysFSGPIO, GPIOBank
from usrp_mpm.sys_utils import i2c_dev
from usrp_mpm.chips.ds125df410 import DS125DF410
from usrp_mpm.periph_manager.common import MboardRegsCommon

# Map register values to SFP transport types
N3XX_SFP_TYPES = {
    0: "",    # Port not connected
    1: "1G",
    2: "10G",
    3: "A",   # Aurora
    4: "W"    # White Rabbit
}

N3XX_FPGA_TYPES_BY_SFP = {
    ("", ""):       "",
    ("1G", "10G"):  "HG",
    ("10G", "10G"): "XG",
    ("10G", "A"):   "XA",
    ("A", "A"):     "AA",
    ("W", "10G"):   "WX",
}

class TCA6424(object):
    """
    Abstraction layer for the port/gpio expander
    pins_list is  an array of different version of TCA6424 pins map.
    First element of this array corresponding to revC, second is revD etc...
    """
    pins_list = [
        (
            'PWREN-CLK-MGT156MHz',
            'NETCLK-CE',         #revC name: 'PWREN-CLK-WB-CDCM',
            'NETCLK-RESETn',     #revC name: 'WB-CDCM-RESETn',
            'NETCLK-PR0',        #revC name: 'WB-CDCM-PR0',
            'NETCLK-PR1',        #revC name: 'WB-CDCM-PR1',
            'NETCLK-OD0',        #revC name: 'WB-CDCM-OD0',
            'NETCLK-OD1',        #revC name: 'WB-CDCM-OD1',
            'NETCLK-OD2',        #revC name: 'WB-CDCM-OD2',
            'PWREN-CLK-MAINREF',
            'CLK-MAINSEL-25MHz', #revC name: 'CLK-MAINREF-SEL1',
            'CLK-MAINSEL-EX_B',  #revC name: 'CLK-MAINREF-SEL0',
            '12',
            'CLK-MAINSEL-GPS',   #revC name: '13',
            'FPGA-GPIO-EN',
            'PWREN-CLK-WB-20MHz',
            'PWREN-CLK-WB-25MHz',
            'GPS-PHASELOCK',
            'GPS-nINITSURV',
            'GPS-nRESET',
            'GPS-WARMUP',
            'GPS-SURVEY',
            'GPS-LOCKOK',
            'GPS-ALARM',
            'PWREN-GPS',
        ),
        (
            'NETCLK-PR1',
            'NETCLK-PR0',
            'NETCLK-CE',
            'NETCLK-RESETn',
            'NETCLK-OD2',
            'NETCLK-OD1',
            'NETCLK-OD0',
            'PWREN-CLK-MGT156MHz',
            'PWREN-CLK-MAINREF',
            'CLK-MAINSEL-25MHz',
            'CLK-MAINSEL-EX_B',
            '12',
            'CLK-MAINSEL-GPS',
            'FPGA-GPIO-EN',
            'PWREN-CLK-WB-20MHz',
            'PWREN-CLK-WB-25MHz',
            'GPS-PHASELOCK',
            'GPS-nINITSURV',
            'GPS-nRESET',
            'GPS-WARMUP',
            'GPS-SURVEY',
            'GPS-LOCKOK',
            'GPS-ALARM',
            'PWREN-GPS',
        )]

    def __init__(self, rev):
        # Default state: Turn on GPS power, take GPS out of reset or
        # init-survey, turn on 156.25 MHz clock
        # min Support from revC or rev = 2
        if rev == 2:
            self.pins = self.pins_list[0]
        else:
            self.pins = self.pins_list[1]

        default_val = 0x860101 if rev == 2 else 0x860780
        self._gpios = SysFSGPIO({'device/name': 'tca6424', 'device/of_node/name': 'gpio'}, 0xFFF7FF, 0x86F7FF, default_val)

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
            0xFFF, # use_mask
            ddr
        )

class BackpanelGPIO(GPIOBank):
    """
    Abstraction layer for the back panel GPIO
    """
    EMIO_BASE = 54
    BP_GPIO_OFFSET = 45
    LED_LINK = 0
    LED_REF = 1
    LED_GPS = 2

    def __init__(self):
        GPIOBank.__init__(
            self,
            {'label': 'zynq_gpio'},
            self.BP_GPIO_OFFSET + self.EMIO_BASE,
            0x7, # use_mask
            0x7, # ddr
        )

class MboardRegsControl(MboardRegsCommon):
    """
    Control the FPGA Motherboard registers
    """
    # pylint: disable=bad-whitespace
    # Motherboard registers (on top of the ones in MboardRegsCommon)
    MB_CLOCK_CTRL     = 0x0018
    MB_XADC_RB        = 0x001C
    MB_BUS_CLK_RATE   = 0x0020
    MB_BUS_COUNTER    = 0x0024
    MB_SFP0_INFO      = 0x0028
    MB_SFP1_INFO      = 0x002C
    MB_GPIO_MASTER    = 0x0030
    MB_GPIO_RADIO_SRC = 0x0034

    # Bitfield locations for the MB_CLOCK_CTRL register.
    MB_CLOCK_CTRL_PPS_SEL_INT_10 = 0 # pps_sel is one-hot encoded!
    MB_CLOCK_CTRL_PPS_SEL_INT_25 = 1
    MB_CLOCK_CTRL_PPS_SEL_EXT    = 2
    MB_CLOCK_CTRL_PPS_SEL_GPSDO  = 3
    MB_CLOCK_CTRL_PPS_SEL_SFP0   = 5
    MB_CLOCK_CTRL_PPS_SEL_SFP1   = 6
    MB_CLOCK_CTRL_PPS_OUT_EN = 4 # output enabled = 1
    MB_CLOCK_CTRL_MEAS_CLK_RESET = 12 # set to 1 to reset mmcm, default is 0
    MB_CLOCK_CTRL_MEAS_CLK_LOCKED = 13 # locked indication for meas_clk mmcm
    MB_CLOCK_CTRL_DISABLE_REF_CLK = 16 # to disable the ref_clk, write a '1'
    # pylint: enable=bad-whitespace

    def __init__(self, label, log):
        MboardRegsCommon.__init__(self, label, log)

    def set_fp_gpio_master(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is a single bit bit mask of 12 pins GPIO
           0: means the pin is driven by PL
           1: means the pin is driven by PS
        """
        with self.regs:
            return self.poke32(self.MB_GPIO_MASTER, value)

    def get_fp_gpio_master(self):
        """get "who" is driving front panel gpio
           The return value is a bit mask of 12 pins GPIO.
           0: means the pin is driven by PL
           1: means the pin is driven by PS
        """
        with self.regs:
            return self.peek32(self.MB_GPIO_MASTER) & 0xfff

    def set_fp_gpio_radio_src(self, value):
        """set driver for front panel GPIO
        Arguments:
            value {unsigned} -- value is 2x12 bits, two bits per GPIO pin
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
           10: means the pin is driven by radio 2
           11: means the pin is driven by radio 3
        """
        with self.regs:
            return self.poke32(self.MB_GPIO_RADIO_SRC, value)

    def get_fp_gpio_radio_src(self):
        """get which radio is driving front panel gpio
           The return value is 2-bit bit mask of 12 pins GPIO.
           00: means the pin is driven by radio 0
           01: means the pin is driven by radio 1
           10: means the pin is driven by radio 2
           11: means the pin is driven by radio 3
        """
        with self.regs:
            return self.peek32(self.MB_GPIO_RADIO_SRC) & 0xffffff

    def set_time_source(self, time_source, ref_clk_freq):
        """
        Set time source
        """
        pps_sel_val = 0x0
        if time_source == 'internal':
            assert ref_clk_freq in (10e6, 25e6), \
                "Invalid reference frequency for time source 'internal'. Must " \
                "be either 10 MHz or 25 MHz. Check clock and time source match."
            if ref_clk_freq == 10e6:
                self.log.debug("Setting time source to internal "
                               "(10 MHz reference)...")
                pps_sel_val = 0b1 << self.MB_CLOCK_CTRL_PPS_SEL_INT_10
            elif ref_clk_freq == 25e6:
                self.log.debug("Setting time source to internal "
                               "(25 MHz reference)...")
                pps_sel_val = 0b1 << self.MB_CLOCK_CTRL_PPS_SEL_INT_25
        elif time_source == 'external':
            self.log.debug("Setting time source to external...")
            pps_sel_val = 0b1 << self.MB_CLOCK_CTRL_PPS_SEL_EXT
        elif time_source == 'gpsdo':
            self.log.debug("Setting time source to gpsdo...")
            pps_sel_val = 0b1 << self.MB_CLOCK_CTRL_PPS_SEL_GPSDO
        elif time_source == 'sfp0':
            self.log.debug("Setting time source to sfp0...")
            pps_sel_val = 0b1 << self.MB_CLOCK_CTRL_PPS_SEL_SFP0
        elif time_source == 'sfp1':
            self.log.debug("Setting time source to sfp1...")
            pps_sel_val = 0b1 << self.MB_CLOCK_CTRL_PPS_SEL_SFP1
        else:
            raise RuntimeError("Invalid time source: {}".format(time_source))

        with self.regs:
            reg_val = self.peek32(self.MB_CLOCK_CTRL) & 0xFFFFFF90
            # prevent glitches by writing a cleared value first, then the final value.
            self.poke32(self.MB_CLOCK_CTRL, reg_val)
            reg_val = reg_val | (pps_sel_val & 0x6F)
            self.log.trace("Writing MB_CLOCK_CTRL to 0x{:08X}".format(reg_val))
            self.poke32(self.MB_CLOCK_CTRL, reg_val)

    def enable_pps_out(self, enable):
        """
        Enables the PPS/Trig output on the back panel
        """
        self.log.trace("%s PPS/Trig output!",
                       "Enabling" if enable else "Disabling")
        mask = 0xFFFFFFFF ^ (0b1 << self.MB_CLOCK_CTRL_PPS_OUT_EN)
        with self.regs:
            # mask the bit to clear it:
            reg_val = self.peek32(self.MB_CLOCK_CTRL) & mask
            if enable:
                # set the bit if desired:
                reg_val = reg_val | (0b1 << self.MB_CLOCK_CTRL_PPS_OUT_EN)
            self.log.trace("Writing MB_CLOCK_CTRL to 0x{:08X}".format(reg_val))
            self.poke32(self.MB_CLOCK_CTRL, reg_val)

    def enable_ref_clk(self, enable):
        """
        Enables the reference clock internal to the FPGA
        """
        self.log.trace("%s the Reference Clock!",
                       "Enabling" if enable else "Disabling")
        mask = 0xFFFFFFFF ^ (0b1 << self.MB_CLOCK_CTRL_DISABLE_REF_CLK)
        with self.regs:
            # mask the bit to clear it and therefore enable the clock:
            reg_val = self.peek32(self.MB_CLOCK_CTRL) & mask
            if not enable:
                # set the bit if not enabled (note this is a DISABLE bit when = 1):
                reg_val = reg_val | (0b1 << self.MB_CLOCK_CTRL_DISABLE_REF_CLK)
            self.log.trace("Writing MB_CLOCK_CTRL to 0x{:08X}".format(reg_val))
            self.poke32(self.MB_CLOCK_CTRL, reg_val)

    def reset_meas_clk_mmcm(self, reset=True):
        """
        Reset or unreset the MMCM for the measurement clock in the FPGA TDC.
        """
        self.log.trace("%s measurement clock MMCM reset...",
                       "Asserting" if reset else "Clearing")
        mask = 0xFFFFFFFF ^ (0b1 << self.MB_CLOCK_CTRL_MEAS_CLK_RESET)
        with self.regs:
            # mask the bit to clear it
            reg_val = self.peek32(self.MB_CLOCK_CTRL) & mask
            if reset:
                # set the bit if desired
                reg_val = reg_val | (0b1 << self.MB_CLOCK_CTRL_MEAS_CLK_RESET)
            self.log.trace("Writing MB_CLOCK_CTRL to 0x{:08X}".format(reg_val))
            self.poke32(self.MB_CLOCK_CTRL, reg_val)

    def get_meas_clock_mmcm_lock(self):
        """
        Check the status of the MMCM for the measurement clock in the FPGA TDC.
        """
        mask = 0b1 << self.MB_CLOCK_CTRL_MEAS_CLK_LOCKED
        with self.regs:
            reg_val = self.peek32(self.MB_CLOCK_CTRL)
        locked = (reg_val & mask) > 0
        if not locked:
            self.log.warning("Measurement clock MMCM reporting unlocked. "
                             "MB_CLOCK_CTRL reg: 0x{:08X}".format(reg_val))
        else:
            self.log.trace("Measurement clock MMCM locked!")
        return locked

    def get_fpga_type(self):
        """
        Reads the type of the FPGA image currently loaded
        Returns a string with the type (ie HG, XG, AA, etc.)
        """
        with self.regs:
            sfp0_info_rb = self.peek32(self.MB_SFP0_INFO)
            sfp1_info_rb = self.peek32(self.MB_SFP1_INFO)
        # Print the registers values as 32-bit hex values
        self.log.trace("SFP0 Info: 0x{0:0{1}X}".format(sfp0_info_rb, 8))
        self.log.trace("SFP1 Info: 0x{0:0{1}X}".format(sfp1_info_rb, 8))
        sfp0_type = N3XX_SFP_TYPES.get((sfp0_info_rb & 0x0000FF00) >> 8, "")
        sfp1_type = N3XX_SFP_TYPES.get((sfp1_info_rb & 0x0000FF00) >> 8, "")
        self.log.trace("SFP types: ({}, {})".format(sfp0_type, sfp1_type))
        try:
            return N3XX_FPGA_TYPES_BY_SFP[(sfp0_type, sfp1_type)]
        except KeyError:
            self.log.warning("Unrecognized SFP type combination: ({}, {})"
                             .format(sfp0_type, sfp1_type))
        return ""


class RetimerQSFP(DS125DF410):
    """
    Thin wrapper around an I2C device that controls the QSFP retimer
    """
    # (deemphasis, swing)
    DRIVER_PRESETS = {
        '1m': (0x00, 0x07), '3m': (0x41, 0x06), 'Optical': (0x41, 0x04)
    }

    def __init__(self, i2c_bus):
        regs_iface = lib.i2c.make_i2cdev_regs_iface(
            i2c_bus,
            0x18,
            False,
            100,
            1
        )
        super(RetimerQSFP, self).__init__(regs_iface)
