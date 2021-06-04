#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from usrp_mpm.sys_utils.db_flash import DBFlash
from usrp_mpm.sys_utils.gpio import Gpio
from usrp_mpm.dboard_manager import DboardIface
from usrp_mpm import lib # Pulls in everything from C++-land

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

    def __init__(self, slot_idx, motherboard):
        super().__init__(slot_idx, motherboard)
        self.db_cpld_iface = motherboard.ctrlport_regs.get_db_cpld_iface(self.slot_idx)
        self._power_enable = Gpio('DB{}_PWR_EN'.format(slot_idx), Gpio.OUTPUT)
        self._power_status = Gpio('DB{}_PWR_STATUS'.format(slot_idx), Gpio.INPUT)

        self.db_flash = DBFlash(slot_idx, log=self.log)

    def tear_down(self):
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
    # Management Bus
    ####################################################################

    ####################################################################
    # Calibration SPI
    #   The SPI/QSPI node used to interact with the DB
    #   Calibration EEPROM if it exists
    ####################################################################
    def get_cal_eeprom_spi_node(self, addr):
        """
        Returns the QSPI node leading to the calibration EEPROM of the
        given DB.
        """
        chip_select = self.mboard.qspi_cs.get(self.db_name, None)
        if chip_select is None:
            raise RuntimeError('No QSPI chip select corresponds ' \
                               'with daughterboard {}'.format(self.db_name))
        return self.mboard.qspi_nodes[chip_select]

    ####################################################################
    # MB Control
    #   Some of the MB settings may be controlled from the DB Driver
    ####################################################################
    def _find_converters(self, direction='both', channel='both'):
        """
        Returns a list of (tile_id, block_id, is_dac) tuples describing
        the data converters associated with a given channel and direction.
        """
        return self.mboard.rfdc._find_converters(self.slot_idx, direction, channel)

    def set_if_freq(self, freq, direction='both', channel='both'):
        """
        Use the rfdc_ctrl object to set the IF frequency of the ADCs and
        DACs corresponding to the specified channels of the DB.
        By default, all channels and directions will be set.
        Returns True if the IF frequency was successfully set.
        """
        for tile_id, block_id, is_dac in self._find_converters(direction, channel):
            if not self.mboard.rfdc._rfdc_ctrl.set_if(tile_id, block_id, is_dac, freq):
                return False
        return True

    def get_if_freq(self, direction, channel):
        """
        Gets the IF frequency of the ADC/DAC corresponding
        to the specified channel of the DB.
        """
        converters = self._find_converters(direction, channel)
        assert len(converters) == 1, \
            'Expected a single RFDC associated with {}{}. Instead found {}.' \
            .format(direction, channel, len(converters))
        (tile_id, block_id, is_dac) = converters[0]
        return self.mboard.rfdc._rfdc_ctrl.get_nco_freq(tile_id, block_id, is_dac)

    def enable_iq_swap(self, enable, direction, channel):
        """
        Enable or disable swap of I and Q samples from the RFDCs.
        """
        for tile_id, block_id, is_dac in self._find_converters(direction, channel):
            self.mboard.rfdc._rfdc_regs.enable_iq_swap(enable, self.slot_idx, block_id, is_dac)

    def get_sample_rate(self):
        """
        Gets the sample rate of the RFDCs.
        """
        return self.mboard.get_spll_freq()

    def get_prc_rate(self):
        """
        Returns the rate of the PLL Reference Clock (PRC) which is
        routed to the daughterboard.
        Note: The ref clock will change if the sample clock frequency
        is modified.
        """
        return self.mboard.get_prc_rate()
