#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Unit test for uhd.types.*Range
"""

import os
import sys
import unittest
import importlib
import pathlib

class VerifyFBSTest(unittest.TestCase):
    """ Test Python-wrapped ranges classes """
    def test_flatbuffer_files(self):
        """ Check FBS file """
        utils_path = os.path.normpath(os.path.join(
            pathlib.Path(__file__).parent.absolute(),
            '..', 'utils', 'update_fbs.py'))
        print(utils_path)
        spec = importlib.util.spec_from_file_location("update_fbs", utils_path)
        update_fbs = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(update_fbs)
        try:
            git_exe = update_fbs.find_executable("git")
        # pylint: disable=broad-except
        except Exception:
            # No git, no test. We pass b/c git is not a UHD dependency.
            return
        sys.argv.append('--verify')
        self.assertTrue(update_fbs.verify(git_exe))
