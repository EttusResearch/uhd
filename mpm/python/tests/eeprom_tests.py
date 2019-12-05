#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

"""
TLV eeprom tests
"""

import os
import struct
from base_tests import TestBase
import usrp_mpm.eeprom
import usrp_mpm.tlv_eeprom
from usrp_mpm.eeprom import MboardEEPROM
from usrp_mpm.periph_manager.base import PeriphManagerBase

def get_eeprom_filename(name):
    """
    location of EEPROM test file relative to test
    """
    return os.path.join(os.path.dirname(__file__), "eeprom_tests", name)

class TestEeprom(TestBase):
    """
    Tests EEPROM readers of different versions. Each reader is expected to
    return a dictionary with values read by the reader. The tests pass
    the dictionary to _check_data which checks equality of all data as well as
    that both dicts contain the same keys.
    """

    # tag map for tlv eeprom tests
    tagmap = {
        # typical board map
        0x42: usrp_mpm.tlv_eeprom.NamedStruct(
            '< H H H 7s 1x', ['pid', 'rev', 'rev_compat', 'serial']),
        # tow additional maps to test unpack of multiple maps in a single eeprom
        0x47: usrp_mpm.tlv_eeprom.NamedStruct(
            '< B B H', ['byte1', 'byte2', 'short']),
        0x11: usrp_mpm.tlv_eeprom.NamedStruct(
            '< L', ['long'])
        }
    magic = 0xCEF10F00

    def _check_data(self, data, **kwargs):
        #generic data check with kwargs containing key-value pairs
        self.assertEqual(len(kwargs), len(data),
                         "test does not check all items "
                         "(Expected: %s Read: %s)" %
                         (kwargs.keys(), data.keys()))
        for key, value in kwargs.items():
            self.assertTrue(key in data, "data does not contain %s" % key)
            self.assertEqual(data[key], value,
                             "%s does not match. Expected: %s, Received: %s" %
                             (key, value, data[key]))

    def test_eeprom_legacy_minsize(self):
        """
        check that empty eeprom files raises struct error
        """
        self.assertRaises(struct.error,
                          usrp_mpm.eeprom.read_eeprom,
                          get_eeprom_filename("empty.eeprom"),
                          0,
                          MboardEEPROM.eeprom_header_format,
                          MboardEEPROM.eeprom_header_keys,
                          PeriphManagerBase.dboard_eeprom_magic)

    def test_eeprom_mboard_v1(self):
        """
        read valid v1 map
        """
        keys = MboardEEPROM.eeprom_header_keys
        (data, _) = usrp_mpm.eeprom.read_eeprom(
            get_eeprom_filename("legacy_mboard_v1.eeprom"),
            0,
            MboardEEPROM.eeprom_header_format,
            keys,
            PeriphManagerBase.mboard_eeprom_magic)
        self._check_data(
            data,
            magic=PeriphManagerBase.mboard_eeprom_magic,
            eeprom_version=1,
            mcu_flags=b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00',
            pid=0xDEAD,
            rev=0xABCD,
            serial=b"mbrd_v1",
            mac_addresses=b'\x01\x02\x03\x04\x05\x06\x00\x00'
                          b'\x0f\x0e\r\x0c\x0b\n\x00\x00\x11'
                          b'"3DUf\x00\x00',
            CRC=0x02CC50AA)

    def test_eeprom_mboard_v2(self):
        """
        read valid v2 map
        """
        (data, _) = usrp_mpm.eeprom.read_eeprom(
            get_eeprom_filename("legacy_mboard_v2.eeprom"),
            0,
            MboardEEPROM.eeprom_header_format,
            MboardEEPROM.eeprom_header_keys,
            PeriphManagerBase.mboard_eeprom_magic)
        self._check_data(
            data,
            magic=PeriphManagerBase.mboard_eeprom_magic,
            eeprom_version=2,
            mcu_flags=b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00',
            pid=0xDEAD,
            rev=0xBEEF,
            serial=b"mbrd_v2",
            mac_eth0=b'\x01\x02\x03\x04\x05\x06',
            dt_compat=0xD1C0,
            mac_eth1=b'\x0f\x0e\r\x0c\x0b\n',
            ec_compat=0xECC0,
            mac_eth2=b'\x11"3DUf',
            CRC=0x42097DAC)

    def test_eeprom_mboard_v3(self):
        """
        read valid v3 map
        """
        (data, _) = usrp_mpm.eeprom.read_eeprom(
            get_eeprom_filename("legacy_mboard_v3.eeprom"),
            0,
            MboardEEPROM.eeprom_header_format,
            MboardEEPROM.eeprom_header_keys,
            PeriphManagerBase.mboard_eeprom_magic)
        self._check_data(
            data,
            magic=PeriphManagerBase.mboard_eeprom_magic,
            eeprom_version=3,
            mcu_flags=b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00',
            pid=0xDEAD,
            rev=0xBEEF,
            serial=b"mbrd_v3",
            mac_eth0=b'\x01\x02\x03\x04\x05\x06',
            dt_compat=0xD1C0,
            mac_eth1=b'\x0f\x0e\r\x0c\x0b\n',
            ec_compat=0xECC0,
            mac_eth2=b'\x11"3DUf',
            rev_compat=0x4EC0,
            CRC=0x2E52CB41)

    def test_eeprom_tlv_single(self):
        """
        read eeprom with tag map
        """
        (data, _) = usrp_mpm.tlv_eeprom.read_eeprom(
            get_eeprom_filename("tlv_single.eeprom"),
            self.tagmap,
            self.magic)
        self._check_data(
            data,
            rev=2,
            rev_compat=2,
            pid=0x410,
            serial=b'3196D29')

    def test_eeprom_tlv_multiple(self):
        """
        read eeprom with multiple tag maps
        """
        (data, _) = usrp_mpm.tlv_eeprom.read_eeprom(
            get_eeprom_filename("tlv_multiple.eeprom"),
            self.tagmap,
            self.magic)
        self._check_data(
            data,
            rev=2,
            rev_compat=2,
            pid=0x410,
            serial=b'3196D29',
            byte1=0x08,
            byte2=0x15,
            short=0xEFBE,
            long=0xBEBAFECA)

    def test_eeprom_tlv_unknown_tagmap(self):
        """
        Check that reading unknown tag map ID leads to empty data map instead
        of runtime error
        """
        (data, _) = usrp_mpm.tlv_eeprom.read_eeprom(
            get_eeprom_filename("tlv_unknown_tagmap.eeprom"),
            self.tagmap,
            self.magic)
        self._check_data(data)

    def test_eeprom_tlv_unknown_wrong_maplen(self):
        """
        Check that invalid map len results in TypeError
        """
        self.assertRaises(TypeError,
                          usrp_mpm.tlv_eeprom.read_eeprom,
                          get_eeprom_filename("tlv_wrong_maplen.eeprom"),
                          self.tagmap,
                          self.magic)
