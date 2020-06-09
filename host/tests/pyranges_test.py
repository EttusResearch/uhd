#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Unit test for uhd.types.*Range
"""

import unittest
import uhd

class RangesTest(unittest.TestCase):
    """ Test Python-wrapped ranges classes """
    def test_meta_range(self):
        """ Test MetaRange.clip() """
        my_range = uhd.types.MetaRange(1.0, 10.0, 0.5)
        self.assertEqual(my_range.clip(5.0), 5.0)
        self.assertEqual(my_range.clip(11.0), 10.0)
        self.assertEqual(my_range.clip(5.1, True), 5.0)
