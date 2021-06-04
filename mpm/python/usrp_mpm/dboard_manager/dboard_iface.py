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

    def tear_down(self):
        """
        Tear down all members that need to be specially handled before
        deconstruction.
        """
        # The mboard object is the periph_manager that has the dboard
        # that in turn has this DboardIface. Breaking the reference
        # cycle will make garbage collection easier.
        self.mboard = None

    ####################################################################
    # Power
    #   Enable and disable the DB's power rails
    ####################################################################
    def enable_daughterboard(self, enable = True):
        """
        Enable or disable the daughterboard.
        """
        raise NotImplementedError('DboardIface::enable_daughterboard() not supported!')

    def check_enable_daughterboard(self):
        """
        Return the enable state of the daughterboard.
        """
        raise NotImplementedError('DboardIface::check_enable_daughterboard() not supported!')

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

    def get_if_freq(self, direction, channel):
        """
        Gets the IF frequency of the ADC/DAC corresponding
        to the specified channel of the DB.
        """
        raise NotImplementedError('DboardIface::get_if_freq() not supported!')

    def enable_iq_swap(self, enable, direction, channel):
        """
        Enable or disable swap of I and Q samples from the RFDCs.
        """
        raise NotImplementedError('DboardIface::enable_iq_swap() not supported!')

    def get_sample_rate(self):
        """
        Gets the sample rate of the RFDCs.
        """
        raise NotImplementedError('DboardIface::get_sample_rate() not supported!')

    def get_prc_rate(self):
        """
        Returns the rate of the PLL Reference Clock (PRC) which is
        routed to the daughterboard.
        """
        raise NotImplementedError('DboardIface::get_pll_ref_clock() not supported!')
