#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Tests the components classes (currently ZynqComponents)
"""

from usrp_mpm.components import ZynqComponents
from base_tests import TestBase

import copy
import os.path
import tempfile
import unittest

class TestZynqComponents(TestBase):
    """
    Test functions of the ZynqComponents class
    """

    _testcase_input = '// mpm_version foo_current_version 1.10\n' \
                      '// mpm_version foo_oldest_compatible_version 1.5\n' \
                      '// mpm_version bar_current_version 1.2\n' \
                      '// mpm_version bar_oldest_compatible_version 1.0\n' \
                      '// mpm_version baz_current_version 1.2.3\n' \
                      '// mpm_version baz_oldest_compatible_version 1.0.0\n' \
                      '// mpm_version zack 2.0\n' \
                      '// mpm_version zack_oldest_compatible_version 1.0\n' \
                      '// mpm_other_tag noname_current_version 1.2.3\n' \
                      '// other comment\n'

    _testcase_result = {
        'bar': {'current': (1, 2), 'oldest': (1,0)},
        'baz': {'current': (1, 2, 3), 'oldest': (1,0,0)},
        'foo': {'current': (1, 10), 'oldest': (1, 5)},
        'zack': {'current': (2, 0), 'oldest': (1,0)},
    }

    def _write_dts_file_from_test_cases(self, content):
        """ Write content to a temporary .dts file """
        f = tempfile.NamedTemporaryFile(mode="w+", suffix=".dts")
        expected = {}
        f.write(content)
        f.flush()
        return f

    def test_parse_dts_version_info_from_file(self):
        """ Test function ZynqComponents._parse_dts_version_info_from_file """
        f = self._write_dts_file_from_test_cases(self._testcase_input)
        expected = self._testcase_result
        result = ZynqComponents._parse_dts_version_info_from_file(f.name)
        self.assertEqual(result, expected)

    def test_verify_compatibility(self):
        """ Test function ZynqComponents._verify_compatibility """
        class _log_dummy():
            def _dummy(self, *args):
                pass
            trace = _dummy
            info = _dummy
            warning = _dummy
            error = _dummy

        f = self._write_dts_file_from_test_cases(self._testcase_input)
        compatibility = self._testcase_result
        for version_type in ['current', 'oldest']:
            for case in ['normal', 'smaller_mpm_minor', 'bigger_mpm_minor',
                'smaller_mpm_major', 'bigger_mpm_major', 'component_missing',
                'additional_component']:
                compatibility_testcase = copy.deepcopy(compatibility)
                foo_major, foo_minor = compatibility['foo'][version_type]
                if case == 'normal':
                    compatibility_testcase['foo'][version_type] = (foo_major, foo_minor)
                    error_expected = None
                elif case == 'smaller_mpm_minor':
                    compatibility_testcase['foo'][version_type] = (foo_major, foo_minor-1)
                    error_expected = None
                elif case == 'bigger_mpm_minor':
                    compatibility_testcase['foo'][version_type] = (foo_major, foo_minor+1)
                    error_expected = None
                elif case == 'smaller_mpm_major':
                    compatibility_testcase['foo'][version_type] = (foo_major-1, foo_minor)
                    if version_type == 'oldest':
                        error_expected = None
                    else:
                        error_expected = RuntimeError()
                elif case == 'bigger_mpm_major':
                    compatibility_testcase['foo'][version_type] = (foo_major+1, foo_minor)
                    if version_type == 'oldest':
                        error_expected = RuntimeError()
                    else:
                        error_expected = None
                elif case == 'component_missing':
                    del compatibility_testcase['foo']
                    error_expected = None
                elif case == 'additional_component':
                    compatibility_testcase['newcomp'] = {version_type: (2, 10)}
                    error_expected = None
                update_dict = {
                    'compatibility': compatibility_testcase,
                    'check_dts_for_compatibility': True,
                }
                filebasename, _ = os.path.splitext(f.name)
                try:
                    self._zynqcomponents = ZynqComponents()
                    self._zynqcomponents.log = _log_dummy()
                    self._zynqcomponents._verify_compatibility(filebasename, update_dict)
                    error = None
                except RuntimeError as r:
                    error = r
                self.assertEqual(error.__class__, error_expected.__class__,
                                 f"Unexpected result for test case {case} (version type: {version_type})")
