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
""" Test the test_messages example. """

import re
from uhd_test_base import uhd_example_test_case

class uhd_test_messages_test(uhd_example_test_case):
    """
    Run test_messages and check output.
    """
    tests = {'default': {},}

    def setup_example(self):
        """
        Set args.
        """
        self.test_params = uhd_test_messages_test.tests

    def run_test(self, test_name, test_args):
        """ Run the app and scrape for the failure messages. """
        self.log.info('Running test {n}'.format(n=test_name,))
        # Run example:
        args = [
            self.create_addr_args_str(),
        ]
        if test_args.has_key('ntests'):
            args.append('--ntests')
            args.append(test_args['ntests'])
        (app, run_results) = self.run_example('test_messages', args)
        # Evaluate pass/fail:
        succ_fail_re = re.compile(r'(?P<test>.*)->\s+(?P<succ>\d+) successes,\s+(?P<fail>\d+) +failures')
        for mo in succ_fail_re.finditer(app.stdout):
            key = mo.group("test").strip().replace(' ', '_').lower()
            successes = int(mo.group("succ"))
            failures = int(mo.group("fail"))
            run_results[key] = "{}/{}".format(successes, successes+failures)
            run_results['passed'] = bool(failures)

        self.report_example_results(test_name, run_results)
        return run_results

