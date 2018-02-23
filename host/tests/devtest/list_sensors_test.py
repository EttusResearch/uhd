#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
""" Test for usrp_list_sensors. """

from uhd_test_base import uhd_example_test_case

class list_sensors_test(uhd_example_test_case):
    """ Run usrp_list_sensors. """
    tests = {'default': {},}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = list_sensors_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the success message. """
        self.log.info('Running test %s', test_name)
        # Run example:
        args = [
            self.create_addr_args_str(),
        ]
        (app, run_results) = self.run_example('usrp_list_sensors', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
            # re.search('All tests passed!', app.stdout) is not None,
        ])
        self.report_example_results(test_name, run_results)
        return run_results

