#!/usr/bin/env python
#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Run the test for tx_burst """

import re
from uhd_test_base import uhd_example_test_case

class uhd_usrp_probe_test(uhd_example_test_case):
    """ Run uhd_usrp_probe """
    tests = {
        'default': {
            'init-only': False,
        },
    }

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = uhd_usrp_probe_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the failure messages. """
        self.log.info('Running test {name}'.format(name=test_name))
        # Run example:
        args = [
            self.create_addr_args_str(),
        ]
        if test_args.get('init-only'):
            args.append('--init-only')
        (app, run_results) = self.run_example('uhd_usrp_probe', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
        ])
        self.report_example_results(test_name, run_results)
        return run_results

