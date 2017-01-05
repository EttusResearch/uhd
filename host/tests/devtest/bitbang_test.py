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
""" Test for gpio --bitbang. """

import re
from uhd_test_base import uhd_example_test_case

class bitbang_test(uhd_example_test_case):
    """ Run gpio --bitbang. """
    tests = {'default': {},}

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
        (app, run_results) = self.run_example('gpio', args)
        # Evaluate pass/fail:
        run_results['passed'] = all([
            app.returncode == 0,
            # re.search('All tests passed!', app.stdout) is not None,
        ])
        self.report_example_results(test_name, run_results)
        return run_results

