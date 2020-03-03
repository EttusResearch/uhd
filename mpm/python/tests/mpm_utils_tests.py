#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
import unittest
from base_tests import TestBase
from usrp_mpm import mpmutils


class MockLockable:
    """
    Class which exposes whether lock() or unlock() have been called on it
    """
    def __init__(self):
        self.locked = False

    def lock(self):
        self.locked = True

    def unlock(self):
        self.locked = False


class TestMpmUtils(TestBase):
    """
    Tests for the myriad utilities in mpmutils
    """
    def test_normal_usage(self):
        """
        Checks whether in normal operation the resource gets unlocked
        """
        my_resource = MockLockable()
        with mpmutils.lock_guard(my_resource):
            self.assertEqual(my_resource.locked, True)
        self.assertEqual(my_resource.locked, False)

    def test_unlocks_after_exception(self):
        """
        Checked whether the resource gets unlocked after an exception occurs
        """
        my_resource = MockLockable()
        try:
            with mpmutils.lock_guard(my_resource):
                self.assertEqual(my_resource.locked, True)
                raise Exception("This is just a drill")
        except Exception:
            # Eat the raised exception
            pass
        finally:
            self.assertEqual(my_resource.locked, False)


if __name__ == '__main__':
    unittest.main()
