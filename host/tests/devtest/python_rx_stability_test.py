#!/usr/bin/env python3
#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test RX Behaviour """

import os
import sys
from uhd_test_base import shell_application
from uhd_test_base import UHDPythonTestCase

class uhd_python_rx_stability_test(UHDPythonTestCase):
    """ Run recv_stability_test """
    test_name = 'uhd_python_rx_stability_test'

    def run_test(self, test_name, test_args):
        """
        Run test and report results.
        """
        devtest_src_dir = os.getenv('_UHD_DEVTEST_SRC_DIR', '')
        args = [
            os.path.join(devtest_src_dir, 'recv_stability_test.py'),
            self.create_addr_args_str(),
        ]
        # The 'app' we are running is just another Python process
        app = shell_application(sys.executable)
        app.run(args)
        run_results = {
            'return_code': app.returncode,
            'passed': False
        }
        run_results['passed'] = all([
            app.returncode == 0,
        ])
        self.log.info('STDERR Output:')
        self.log.info(str(app.stderr))
        for key in sorted(run_results):
            self.log.info('%s = %s', str(key), str(run_results[key]))
            self.report_result(
                "python_rx_tester",
                key, run_results[key]
            )
        if 'passed' in run_results:
            self.report_result(
                "python_rx_tester",
                'status',
                'Passed' if run_results['passed'] else 'Failed',
            )
        return run_results
