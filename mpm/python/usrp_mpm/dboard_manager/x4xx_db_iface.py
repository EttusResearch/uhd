#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4xx Daughterboard interface. See X4xxDboardIface documentation.
"""

from usrp_mpm.sys_utils.db_flash import DBFlash
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.dboard_manager import DboardIface
from usrp_mpm.mpmutils import get_dboard_class_from_pid

class X4xxDboardIface(DboardIface):
    """
    X4xx DboardIface implementation

    slot_idx - The numerical ID of the daughterboard slot using this
               interface (e.g. 0, 1)
    motherboard - The instance of the motherboard class which implements
                  these controls
    """
    # The device tree label for the bus to the DB's Management EEPROM
    MGMT_EEPROM_DEVICE_LABEL = "e0004000.i2c"

    def __init__(self, slot_idx, motherboard, dboard_info):
        super().__init__(slot_idx, motherboard)
        self.db_cpld_iface = motherboard.ctrlport_regs.get_db_cpld_iface(self.slot_idx)
        self._power_enable = Gpio(f'DB{slot_idx}_PWR_EN', Gpio.OUTPUT)
        self._power_status = Gpio(f'DB{slot_idx}_PWR_STATUS', Gpio.INPUT)
        self.db_flash = None
        self._init_db_flash(slot_idx, dboard_info)

    def _init_db_flash(self, slot_idx, dboard_info):
        """
        Identify if this daughterboard has a flash memory, and initialize it if
        necessary.
        """
        db_class = get_dboard_class_from_pid(dboard_info['pid'])
        if getattr(db_class, 'has_db_flash', False):
            self.db_flash = DBFlash(slot_idx, log=self.log)

    def tear_down(self):
        """
        De-init flash memory before shutting down to avoid data loss.
        """
        self.log.trace("Tearing down X4xx daughterboard...")
        if self.db_flash:
            self.db_flash.deinit()
        super().tear_down()

    ####################################################################
    # Power
    #   Enable and disable the DB's power rails
    ####################################################################
    def enable_daughterboard(self, enable=True):
        """
        Enable or disable the daughterboard.
        """
        if self.db_flash and not enable:
            self.db_flash.deinit()
        self._power_enable.set(enable)
        self.mboard.cpld_control.enable_daughterboard(self.slot_idx, enable)
        if self.db_flash and enable:
            self.db_flash.init()

    def check_enable_daughterboard(self):
        """
        Return the enable state of the daughterboard.
        """
        return self._power_status.get()

    ####################################################################
    # CTRL SPI
    #   CTRL SPI lines are connected to the CPLD of the DB if it exists
    ####################################################################
    def peek_db_cpld(self, addr):
        return self.db_cpld_iface.peek32(addr)

    def poke_db_cpld(self, addr, val):
        self.db_cpld_iface.poke32(addr, val)

    ####################################################################
    # MB Control
    #   Some of the MB settings may be controlled from the DB Driver
    ####################################################################
    def enable_iq_swap(self, enable, direction, channel):
        """
        Enable or disable swap of I and Q samples from the RFDCs.
        """
        self.mboard.rfdc.enable_iq_swap(enable, self.slot_idx, channel, direction == 'tx')

    def get_sample_rate(self):
        """
        Gets the sample rate of the RFDCs.
        """
        return self.mboard.clk_mgr.get_converter_rate(self.slot_idx)

    def get_prc_rate(self):
        """
        Returns the rate of the PLL Reference Clock (PRC) which is routed to
        the daughterboard.
        Note: The PRC rate will change if the sample clock frequency is modified.
        """
        return self.mboard.clk_mgr.get_prc_rate()
