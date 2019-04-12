#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

class DboardIface(object):
    """
    An interface through which Motherboard drivers can expose certain
    functionality to Daughterboard drivers within MPM.

    Each Motherboard should have its own instance of this interface,
    implementing the relevant methods.

    slot_idx - The numerical ID of the daughterboard slot using this
               interface (e.g. 0, 1)
    motherboard - The instance of the motherboard class which implements
                  these controls
    """
    # The device tree label for the bus to the DB's Management EEPROM
    MGMT_EEPROM_DEVICE_LABEL = None

    def __init__(self, slot_idx, motherboard):
        self.slot_idx = slot_idx
        self.mboard = motherboard
        self.db_name = "db_{}".format(self.slot_idx)

        if hasattr(self.mboard, 'log'):
            self.log = self.mboard.log.getChild("DboardIface")

    ####################################################################
    # CTRL SPI
    #   CTRL SPI lines are connected to the CPLD of the DB if it exists
    ####################################################################
    def peek_db_cpld(self, addr):
        raise NotImplementedError('DboardIface::peek_db_cpld() not supported!')

    def poke_db_cpld(self, addr, val):
        raise NotImplementedError('DboardIface::poke_db_cpld() not supported!')

    def ctrl_spi_reset(self):
        raise NotImplementedError('DboardIface::ctrl_spi_reset() not supported!')

    ####################################################################
    # GPIO
    #   GPIO lines are used for high speed control of the DB
    ####################################################################
    def get_high_speed_gpio_ctrl_core(self):
        """
        Return a GpioAtrCore4000 instance that controls the GPIO lines
        interfacing the MB and DB
        """
        raise NotImplementedError('DboardIface::get_high_speed_gpio_ctrl_core()'
                                  ' not supported!')

    ####################################################################
    # Management Bus
    ####################################################################

    ####################################################################
    # Calibration SPI
    #   The SPI/QSPI node used to interact with the DB
    #   Calibration EEPROM if it exists
    ####################################################################
    def get_cal_eeprom_spi_node(self, addr):
        raise NotImplementedError('DboardIface::get_cal_eeprom_spi_node()'
                                  ' not supported!')

    ####################################################################
    # MB Control
    #   Some of the MB settings may be controlled from the DB Driver
    ####################################################################
    def set_reference_clock(self, freq):
        raise NotImplementedError('DboardIface::set_reference_clock() not supported!')

    def set_if_freq(self, freq, direction='both', channel='both'):
        """
        Set the IF frequency of the ADCs and DACs corresponding
        to the specified channels of the DB.
        By default, all channels and directions will be set.
        Returns true if the IF frequency was successfully set.
        """
        raise NotImplementedError('DboardIface::set_if_freq() not supported!')

    def get_prc_rate(self):
        """
        Returns the rate of the PLL Reference Clock (PRC) which is
        routed to the daughterboard.
        """
        raise NotImplementedError('DboardIface::get_pll_ref_clock() not supported!')

    ####################################################################
    # SPCC MPCC Control
    ####################################################################
    def get_protocol_cores(self):
        """
        Returns all discovered protocols in SPCC and MPCC blocks on the
        Daughterboard's CPLD in the form of SpiCore4000, I2cCore4000,
        UartCore4000, and GpioAtrCore4000
        """
        raise NotImplementedError('DboardIface::get_protocol_cores() not supported!')
