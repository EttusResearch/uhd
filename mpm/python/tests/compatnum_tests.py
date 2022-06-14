#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import unittest
from base_tests import TestBase
from usrp_mpm.compat_num import CompatNumber

class TestCompatNum(TestBase):

    def test_init(self):
        """
        Checking __init__() methods work...
        """
        cn0 = CompatNumber(4.3)
        cn1 = CompatNumber(4, 3)
        cn2 = CompatNumber((4, 3, 1234))
        self.assertTrue(cn0 == cn1 == cn2)

    def test_ops(self):
        """
        Checking comparison operators...
        """
        c = CompatNumber(4.3)
        self.assertTrue(c < CompatNumber(5, 0))
        self.assertTrue(c == 4.3)
        self.assertTrue(c > 3)
        self.assertTrue(c <= 4.3)
        self.assertTrue(c <= (5, 0))
        self.assertTrue(c >= 3)
        self.assertFalse(c != 4.3)
        self.assertTrue(c == "4.3")
