#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test for gpio --bitbang. """

import re
from uhd_test_base import uhd_example_test_case

class bitbang_test(uhd_example_test_case):
    """ Run gpio --bitbang. """
    tests = {'default': {
        'addl_args': [],
    },}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = bitbang_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the success message. """
        self.log.info('Running test {n}'.format(n=test_name,))
        # Run example:
        args = [
            self.create_addr_args_str(),
            '--bitbang',
        ]
        args += test_args['addl_args']
        (app, run_results) = self.run_example('gpio', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
            # re.search('All tests passed!', app.stdout) is not None,
        ])
        self.report_example_results(test_name, run_results)
        return run_results
