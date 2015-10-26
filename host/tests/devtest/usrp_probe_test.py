#!/usr/bin/env python
#
# Copyright 2015 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

