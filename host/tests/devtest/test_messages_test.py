#!/usr/bin/env python3
#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test the test_messages example. """

import re
from uhd_test_base import uhd_example_test_case

class test_messages_test(uhd_example_test_case):
    """
    Run test_messages and check output.
    """
    tests = {'default': {},}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = test_messages_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the failure messages. """
        self.log.info('Running test %s', test_name)
        # Run example:
        args = [
            self.create_addr_args_str(),
        ]
        if 'ntests' in test_args:
            args.append('--ntests')
            args.append(test_args['ntests'])
        (app, run_results) = self.run_example('test_messages', args)
        # Evaluate pass/fail:
        succ_fail_re = re.compile(r'(?P<test>.*)->\s+(?P<succ>\d+) successes,\s+(?P<fail>\d+) +failures')
        for mobj in succ_fail_re.finditer(app.stdout):
            key = mobj.group("test").strip().replace(' ', '_').lower()
            successes = int(mobj.group("succ"))
            failures = int(mobj.group("fail"))
            run_results[key] = "{}/{}".format(successes, successes+failures)
            run_results['passed'] = (failures == 0)
        self.report_example_results(test_name, run_results)
        return run_results
