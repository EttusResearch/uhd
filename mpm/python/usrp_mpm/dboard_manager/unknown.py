#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
EISCAT rx board implementation module
"""
from usrp_mpm.dboard_manager import DboardManagerBase

class unknown(DboardManagerBase):
    hw_pid = 0
    special_eeprom_addrs = {}

    def __init__(self, spi_devices, eeprom_data):
        self._eeprom = eeprom_data[0] # save eeprom header
        # Do own init
        super(unknown, self).__init__()
