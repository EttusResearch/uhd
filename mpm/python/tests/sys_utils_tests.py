#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tests related to usrp_mpm.sys_utils
"""

from base_tests import TestBase
import unittest
import test_utilities
from usrp_mpm.sys_utils import net
import platform


class TestNet(TestBase):
    """
    Tests multiple functions defined in usrp_mpm.sys_utils.net.

    Some tests are system agnostic and some are only run on
    USRPs with an ARM processor.
    For tests run on the USRP, it is assumed that the device has at
    least an active RJ-45 (eth0) connection.
    """
    def test_get_hostname(self):
        """
        Test net.get_hostname() returns the same value as
        platform.node() which should also be the network hostname of
        the current system.
        """
        expected_hostname = platform.node()
        self.assertEqual(expected_hostname, net.get_hostname())

    @TestBase.skipUnlessOnUsrp()
    def test_get_valid_interfaces(self):
        """
        Test that expected network interfaces are returned as valid
        and that unexpected network interfaces are not.

        This test assumes there is an ethernet connection to the USRP
        RJ-45 connector and will fail otherwise.

        Note: This test is only valid when run on a USRP because the
        network interfaces of a dev machine are unknown.
        """
        expected_valid_ifaces = ['eth0']
        expected_invalid_ifaces = ['eth2', 'spf2']
        all_ifaces = expected_valid_ifaces + expected_invalid_ifaces
        resulting_valid_ifaces = net.get_valid_interfaces(all_ifaces)
        self.assertEqual(expected_valid_ifaces, resulting_valid_ifaces)

    @TestBase.skipUnlessOnUsrp()
    def test_get_iface_info(self):
        """
        Tests the get_iface_info function.
        Expected ifaces should return information in the correct format
        while unexpected ifaces should raise an IndexError.

        Note: This test is only valid when run on a USRP because the
        network interfaces of a dev machine are unknown.
        """
        if self.device_name == 'n3xx':
            possible_ifaces = ['eth0', 'sfp0', 'sfp1']
        elif self.device_name == 'x4xx':
            # x4xx devices have an internal network interface
            # TODO: change this when sfp0 is enabled
            # possible_ifaces = ['eth0', 'sfp0', 'int0']
            possible_ifaces = ['eth0', 'int0']
        else:
            possible_ifaces = ['eth0', 'sfp0']

        active_ifaces = net.get_valid_interfaces(possible_ifaces)

        for iface_name in possible_ifaces:
            iface_info = net.get_iface_info(iface_name)
            # Verify the output info contains the expected keys
            self.assertGreaterEqual(set(iface_info), {'mac_addr', 'ip_addr', 'ip_addrs', 'link_speed'})
            if iface_name in active_ifaces:
                # Verify interfaces with an active connection have a set IPv4 address
                self.assertNotEqual(iface_info['ip_addr'], '')

        unknown_name = 'unknown_iface'
        # Verify that an unknown interface throws a LookupError
        self.assertRaises(LookupError, net.get_iface_info, unknown_name)

    @TestBase.skipUnlessOnUsrp()
    def test_get_link_speed(self):
        """
        Tests that the link speed of 'eth0' is the expected 1GB and that
        when the function is called on unknown interfaces, an exception
        is raised.

        Note: This test is only valid when run on a USRP because the
        network interfaces of a dev machine are unknown.
        """
        known_iface = 'eth0'
        self.assertEqual(1000, net.get_link_speed(known_iface))
        unknown_iface = 'unknown'
        self.assertRaises(IndexError, net.get_link_speed, unknown_iface)

    def test_ip_addr_to_iface(self):
        """
        Tests ip_addr_to_iface to ensure that the iface name is looked
        up properly.
        """
        iface_list = {
            'eth0': {
                'mac_addr': None,
                'ip_addr': '10.2.34.6',
                'ip_addrs': ['10.2.99.99', '10.2.34.6'],
                'link_speed': None,
            },
            'eth1': {
                'mac_addr': None,
                'ip_addr': '10.2.99.99',
                'ip_addrs': ['10.2.99.99'],
                'link_speed': None,
            }
        }
        self.assertEqual(net.ip_addr_to_iface('10.2.34.6', iface_list), 'eth0')
        self.assertEqual(net.ip_addr_to_iface('10.2.99.99', iface_list), 'eth1')
        # TODO: If the IP address cannot be found it should probably not
        # raise a KeyError but instead fail more gracefully
        self.assertRaises(KeyError, net.ip_addr_to_iface, '10.2.100.100', iface_list)

    def test_byte_to_mac(self):
        """
        Test the conversion from byte string to formatted MAC address.
        Compares an expected formatted MAC address with the actual
        returned value.
        """
        mac_addr = 0x2F16ABBF9063
        byte_str = ""
        for byte_index in range(0, 6):
            byte = (mac_addr >> (byte_index * 8)) & 0xFF
            byte_char = chr(byte)
            byte_str = byte_char + byte_str
        expected_string = '2F:16:AB:BF:90:63'
        self.assertEqual(expected_string, net.byte_to_mac(byte_str).upper())

if __name__ == '__main__':
    unittest.main()
