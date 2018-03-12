#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
test periph_manager implementation module
"""

from __future__ import print_function
import random
import string
from usrp_mpm.periph_manager import PeriphManagerBase
from usrp_mpm import dboard_manager


class test(PeriphManagerBase):
    """
    Test periph manager class which fakes out all API calls
    """
    hw_pids = "42"
    mboard_info = {"type": "mpm_test"}
    mboard_eeprom_addr = None
    dboard_eeprom_addrs = {"A": "something", "B": "else"}
    dboard_spimaster_addrs = {"A": "something", "B": "else"}

    def __init__(self, *args, **kwargs):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        # super(n310, self).__init__(*args, **kwargs)
        # if header.get("dataversion", 0) == 1:
        self._eeprom = self._read_eeprom_fake()
        print(self.mboard_info)
        self.mboard_info["serial"] = "AABBCCDDEEFF"
        self.mboard_info["name"] = self._eeprom["name"]

        # I'm the test periph_manager, I know I have test dboards attached
        self.dboards = {
            "A": dboard_manager.test(self._read_db_eeprom_random()),
            "B": dboard_manager.test(self._read_db_eeprom_random())
        }

    def _read_eeprom_fake(self):
        """
        fake eeprom readout function, returns dict with data
        """
        fake_eeprom = {
            "magic": 42,
            "crc": 4242,
            "data_version": 42,
            "hw_pid": 42,
            "hw_rev": 5,
            "name": "foo"
        }

        return fake_eeprom

    def _read_db_eeprom_random(self):
        """
        fake db eeprom readout function, returns dict with fake dboard data
        """
        fake_eeprom = {
            "serial": ''.join(
                random.choice("ABCDEF" + string.digits)
                for _ in range(16))
        }
        return fake_eeprom
