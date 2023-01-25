#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Unit test for uhd.types.DeviceAddr
"""

import unittest
from uhd.types import DeviceAddr, separate_device_addr, combine_device_addrs

class DeviceAddrTest(unittest.TestCase):
    """ Test Python-wrapped device_addr_t """

    def test_device_addr_dict(self):
        """
        Check we can use DeviceAddr like a Python dict
        """
        dev_a = DeviceAddr("a=1,b=2,c=3")
        self.assertEqual(len(dev_a), 3)
        self.assertEqual(dev_a.to_dict(), {'a': '1', 'b': '2', 'c': '3'})
        dev_d = dev_a.to_dict()
        self.assertEqual(dev_a.keys(), list(dev_d.keys()))
        self.assertEqual(dev_a.values(), list(dev_d.values()))
        self.assertEqual(dev_a.get('a'), '1')
        self.assertEqual(dev_a.get('a', 'boo'), '1')
        self.assertEqual(dev_a.get('z'), None)
        self.assertEqual(dev_a.get('z', 'boo'), 'boo')
        self.assertEqual(dev_a['a'], '1')
        with self.assertRaises(KeyError):
            print(dev_a['z'])
        self.assertTrue('a' in dev_a)
        self.assertEqual(dev_a.pop('a'), '1')
        self.assertEqual(len(dev_a), 2)
        self.assertTrue('a' not in dev_a)
        self.assertEqual(dev_a.pop('z', 'boo'), 'boo')
        with self.assertRaises(KeyError):
            print(dev_a.pop('z'))
        dev_a.update({'a': '9'})
        self.assertEqual(dev_a.get('a'), '9')
        with self.assertRaises(RuntimeError):
            dev_a.update({'a': '12'})
        dev_a.update({'a': '12'}, False)
        self.assertEqual(dev_a.get('a'), '12')

    def test_device_addr_separate(self):
        dev_a = DeviceAddr("addr0=192.168.40.1,addr1=192.168.40.2,time_source=external")
        self.assertEqual(len(dev_a.separate()), 2)
        self.assertEqual(len(dev_a.separate()[0]), 2)
        self.assertEqual(dev_a.separate(), separate_device_addr(dev_a))
        self.assertEqual(
            DeviceAddr(
                'addr0=192.168.40.1,time_source0=external,'
                'addr1=192.168.40.2,time_source1=external'),
            combine_device_addrs(dev_a.separate()))
