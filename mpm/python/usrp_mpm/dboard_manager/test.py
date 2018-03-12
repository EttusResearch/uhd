#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
magnesium dboard implementation module
"""

from builtins import object
from usrp_mpm.dboard_manager import DboardManagerBase

class fake_spi(object):
    def __init__(self, addr):
        self.addr = addr


class test_device(object):
    def __init__(self, dev1, dev2, dev3):
        self.dev1 = fake_spi(dev1)
        self.dev2 = fake_spi(dev2)
        self.dev3 = fake_spi(dev3)

    def test_method1(self, argument):
        return argument


class test(DboardManagerBase):
    hw_pid = 234
    special_eeprom_addrs = {"special0": "something"}
    spi_chipselect = {"0": "dev1", "1": "dev2", "2": "dev3"}
    spidevs = {}

    def __init__(self, *args, **kwargs):
        # eeprom_data is a tuple (head_dict, raw_data)
        super(test, self).__init__(*args, **kwargs)
        # I'm the test device, I can fake out my EEPROM
        self.dev1 = "0"
        self.dev2 = "1"
        self.dev3 = "2"

    def init_device(self):
        self.log.debug("initialize hardware")
        self._device = test_device(self.dev1, self.dev2, self.dev3)

