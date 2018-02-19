#!/usr/bin/env python
#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test for test_pps_input. """

import re
from uhd_test_base import uhd_example_test_case

class uhd_test_pps_test(uhd_example_test_case):
    """ Run test_pps_input. """
    tests = {'default': {},}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = uhd_test_pps_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the success message. """
        self.log.info('Running test {n}'.format(n=test_name,))
        # Run example:
        args = [
            self.create_addr_args_str(),
        ]
        if test_args.has_key('source'):
            args.append('--source')
            args.append(test_args['source'])
        (app, run_results) = self.run_example('test_pps_input', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
            re.search('Success!', app.stdout) is not None,
        ])
        self.report_example_results(test_name, run_results)
        return run_results

